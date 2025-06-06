/**
 * @file
 *
 * Copyright 2018-2023 Alexey Gruzdev
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "PluginSvgCairo.h"

#include <QDebug>
#include <QSvgRenderer>
#include <QImage>
#include <QPainter>
#include <QRect>
#include <QFileInfo>
#include <QStringList>
#include <iostream>
#include "FreeImageExt.h"


#ifdef _WIN32
# include <Windows.h>
# define LIBRARY_HANDLE_TYPE HMODULE
# define SET_LOAD_DIRECTORY(QString_) SetDllDirectory((QString_).toStdWString().c_str())
# define RESTORE_LOAD_DIRECTORY SetDllDirectory(NULL)
# define LOAD_LIBRARY(QString_) LoadLibraryW((QString_).toStdWString().c_str())
# define FREE_LIBRARY(Handle_) FreeLibrary(Handle_)
# define LOAD_SYMBOL(Handle_, Symbol_) GetProcAddress(Handle_, Symbol_)
#else
# include <dlfcn.h>
# define LIBRARY_HANDLE_TYPE void*
# define SET_LOAD_DIRECTORY(QString_)
# define RESTORE_LOAD_DIRECTORY
# define LOAD_LIBRARY(QString_) dlopen((QString_).toStdString().c_str(), RTLD_NOW | RTLD_GLOBAL)
# define FREE_LIBRARY(Handle_) dlclose(Handle_)
# define LOAD_SYMBOL(Handle_, Symbol_) dlsym(Handle_, Symbol_)
#endif


namespace
{
    // https://www.manpagez.com/html/glib/glib-2.56.0/glib-Error-Reporting.php#GError
    struct GError {
        uint32_t       domain;
        int32_t         code;
        char* message;
    };

    // https://gnome.pages.gitlab.gnome.org/librsvg/Rsvg-2.0/struct.Rectangle.html
    struct RsvgRectangle {
        double x;
        double y;
        double width;
        double height;
    };

    // https://www.manpagez.com/html/rsvg-2.0/rsvg-2.0-2.40.20/RsvgHandle.php#RsvgDimensionData
    struct RsvgDimensionData {
        int width;
        int height;
        double em;
        double ex;
    };

    struct LibVersion
    {
        uint32_t major{};
        uint32_t minor{};
        uint32_t micro{};

        QString toQString() const {
            return QString::number(major) + "." + QString::number(minor) + "." + QString::number(micro);
        }
    };


    class LibraryLoader
    {
    public:
        LibraryLoader(const QStringList& names) {
            for (const auto& n : names) {
                QFileInfo libPathInfo{ n };
                SET_LOAD_DIRECTORY(libPathInfo.absolutePath());
                mHandle = LOAD_LIBRARY(libPathInfo.absoluteFilePath());
                if (mHandle) {
                    break;
                }
            }
            RESTORE_LOAD_DIRECTORY;
            if (!mHandle) {
                throw std::runtime_error("Failed to open library.");
            }
        }

        LibraryLoader(const QString& name)
            : LibraryLoader(QStringList{ name })
        { }

        LibraryLoader(const LibraryLoader&) = delete;
        LibraryLoader(LibraryLoader&&) = delete;

        virtual ~LibraryLoader() {
            FREE_LIBRARY(mHandle);
        }

        LibraryLoader& operator=(const LibraryLoader&) = delete;
        LibraryLoader& operator=(LibraryLoader&&) = delete;


        template <typename SymbolType_>
        SymbolType_ LoadSymbol(const char* name, bool required = true) {
            SymbolType_ sym = static_cast<SymbolType_>(static_cast<void*>(LOAD_SYMBOL(mHandle, name)));
            if (required && !sym) {
                throw std::runtime_error(std::string("Failed to load symbol '") + name + "'");
            }
            return sym;
        }

    private:
        LIBRARY_HANDLE_TYPE mHandle{};
    };

    std::unique_ptr<QByteArray> loadXmlBuffer(FreeImageIO* io, fi_handle handle)
    {
        const auto posStart = io->tell_proc(handle);
        io->seek_proc(handle, 0, SEEK_END);
        const auto posEnd = io->tell_proc(handle);
        io->seek_proc(handle, posStart, SEEK_SET);

        if (posEnd <= posStart) {
            return nullptr;
        }

        const auto xmlSize = static_cast<uint32_t>(posEnd - posStart);
        auto xmlBuffer = std::make_unique<QByteArray>(xmlSize, Qt::Initialization::Uninitialized);
        if (xmlSize != io->read_proc(xmlBuffer->data(), sizeof(char), xmlSize, handle)) {
            return nullptr;
        }

        return xmlBuffer;
    }
}


class PluginSvgCairo::LibRsvg
    : public LibraryLoader
{
public:
    LibVersion version{};

    void* (*rsvg_handle_new_from_data_f)(const char*, unsigned long, GError**) { nullptr };
    void  (*rsvg_handle_get_dimensions_f)(void*, RsvgDimensionData*) { nullptr };
    int   (*rsvg_handle_render_document_f)(void*, void*, const RsvgRectangle*, GError**) { nullptr };
    int   (*rsvg_handle_close_f)(void* handle, GError** error) { nullptr };
    void  (*rsvg_handle_free_f)(void* handle) { nullptr };


    LibRsvg(const QString& libRsvgPath)
        try
        : LibraryLoader(libRsvgPath)
    {
        version.major = *LoadSymbol<uint32_t*>("rsvg_major_version");
        version.minor = *LoadSymbol<uint32_t*>("rsvg_minor_version");
        version.micro = *LoadSymbol<uint32_t*>("rsvg_micro_version");

        // https://www.manpagez.com/html/rsvg-2.0/rsvg-2.0-2.42.2/rsvg-RsvgHandle.php#rsvg-handle-new-from-file
        rsvg_handle_new_from_data_f = LoadSymbol<decltype(rsvg_handle_new_from_data_f)>("rsvg_handle_new_from_data");

        // https://www.manpagez.com/html/rsvg-2.0/rsvg-2.0-2.40.20/RsvgHandle.php#rsvg-handle-get-dimensions
        rsvg_handle_get_dimensions_f = LoadSymbol<decltype(rsvg_handle_get_dimensions_f)>("rsvg_handle_get_dimensions");

        // https://gnome.pages.gitlab.gnome.org/librsvg/Rsvg-2.0/method.Handle.render_document.html
        rsvg_handle_render_document_f = LoadSymbol<decltype(rsvg_handle_render_document_f)>("rsvg_handle_render_document");

        // https://www.manpagez.com/html/rsvg-2.0/rsvg-2.0-2.42.2/rsvg-RsvgHandle.php#rsvg-handle-close
        rsvg_handle_close_f = LoadSymbol<decltype(rsvg_handle_close_f)>("rsvg_handle_close");

        // https://www.manpagez.com/html/rsvg-2.0/rsvg-2.0-2.42.2/rsvg-RsvgHandle.php#rsvg-handle-free
        rsvg_handle_free_f = LoadSymbol<decltype(rsvg_handle_free_f)>("rsvg_handle_free");
    }
    catch (std::exception& err) {
        throw std::runtime_error(std::string("PluginSvgCairo[ctor]: Failed to load librsvg. Reason: ") + err.what());
    }
};

class PluginSvgCairo::LibCairo
    : public LibraryLoader
{
public:
    LibVersion version{};

    void* (*cairo_image_surface_create_for_data_f)(void*, int, int, int, int) { nullptr };
    int   (*cairo_surface_status_f)(void*) { nullptr };
    void* (*cairo_create_f)(void*) { nullptr };
    void  (*cairo_translate_f)(void*, double, double) { nullptr };
    void  (*cairo_scale_f)(void*, double, double) { nullptr };
    void  (*cairo_surface_destroy_f)(void*) { nullptr };
    void  (*cairo_destroy_f)(void*) { nullptr };


    LibCairo(const QString& libCairoPath)
        try
        : LibraryLoader(libCairoPath)
    {
        const int cairoVersionValue = LoadSymbol<int(*)()>("cairo_version")();
        version.micro = cairoVersionValue % 100;
        version.minor = cairoVersionValue / 100 % 100;
        version.major = cairoVersionValue / 10000;

        // https://www.cairographics.org/manual/cairo-Image-Surfaces.html#cairo_image_surface_create_for_data
        cairo_image_surface_create_for_data_f = LoadSymbol<decltype(cairo_image_surface_create_for_data_f)>("cairo_image_surface_create_for_data");

        // https://www.cairographics.org/manual/cairo-Image-Surfaces.html#cairo-image-surface-create
        // https://github.com/ImageMagick/cairo/blob/5633024ccf6a34b6083cba0a309955a91c619dff/src/cairo.h#L441
        cairo_surface_status_f = LoadSymbol<decltype(cairo_surface_status_f)>("cairo_surface_status");

        // https://www.cairographics.org/manual/cairo-cairo-t.html#cairo-create
        cairo_create_f = LoadSymbol<decltype(cairo_create_f)>("cairo_create");

        // https://www.cairographics.org/manual/cairo-Transformations.html
        cairo_translate_f = LoadSymbol<decltype(cairo_translate_f)>("cairo_translate");
        cairo_scale_f     = LoadSymbol<decltype(cairo_scale_f)>("cairo_scale");

        // https://www.cairographics.org/manual/cairo-cairo-surface-t.html#cairo-surface-destroy
        cairo_surface_destroy_f = LoadSymbol<decltype(cairo_surface_destroy_f)>("cairo_surface_destroy");

        // https://www.cairographics.org/manual/cairo-cairo-t.html#cairo-destroy
        cairo_destroy_f = LoadSymbol<decltype(cairo_destroy_f)>("cairo_destroy");
    }
    catch (std::exception& err) {
        throw std::runtime_error(std::string("PluginSvgCairo[ctor]: Failed to load libcairo. Reason: ") + err.what());
    }
};



PluginSvgCairo::PluginSvgCairo(const QString& libCairoPath, const QString& libRsvgPath)
{
    mLibRsvg  = std::make_unique<LibRsvg>(libRsvgPath);
    mLibCairo = std::make_unique<LibCairo>(libCairoPath);

    //auto err = GetLastError();
    //wchar_t* msg = nullptr;
    //FormatMessage(
    //    FORMAT_MESSAGE_ALLOCATE_BUFFER |
    //    FORMAT_MESSAGE_FROM_SYSTEM |
    //    FORMAT_MESSAGE_IGNORE_INSERTS,
    //    NULL,
    //    err,
    //    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    //    (LPTSTR)&msg,
    //    0, NULL);
}


PluginSvgCairo::~PluginSvgCairo() = default;


const char* PluginSvgCairo::FormatProc()
{
    return "SVG";
}


const char* PluginSvgCairo::DescriptionProc()
{
    return "Scalable Vector Graphics";
}


const char* PluginSvgCairo::ExtensionListProc()
{
    return "svg";
}


FIBITMAP* PluginSvgCairo::LoadProc(FreeImageIO* io, fi_handle handle, uint32_t page, uint32_t flags, void* data)
{
    try {
        //const auto t0 = std::chrono::steady_clock::now();

        const auto xmlBuffer = loadXmlBuffer(io, handle);
        if (!xmlBuffer) {
            throw std::runtime_error("Failed to read xml buffer");
        }

        GError* rsvgError = nullptr;
        auto rsvgHandle = mLibRsvg->rsvg_handle_new_from_data_f(xmlBuffer->data(), xmlBuffer->size(), &rsvgError);
        if (!rsvgHandle) {
            throw std::runtime_error("Failed to create rsvg handle");
        }
        auto cleanupRsvgHandle = qScopeGuard([&, this] { mLibRsvg->rsvg_handle_free_f(rsvgHandle); });

        RsvgDimensionData dims{};
        mLibRsvg->rsvg_handle_get_dimensions_f(rsvgHandle, &dims);
        if (dims.width * dims.height <= 0) {
            dims.width  = 1024;
            dims.height = 1024;
        }

        std::unique_ptr<FIBITMAP, decltype(&::FreeImage_Unload)> image(FreeImage_Allocate(dims.width, dims.height, 32), &::FreeImage_Unload);

        auto cairoSurface = mLibCairo->cairo_image_surface_create_for_data_f(FreeImage_GetBits(image.get()), 0, dims.width, dims.height, FreeImage_GetPitch(image.get()));
        if (0 != mLibCairo->cairo_surface_status_f(cairoSurface)) {
            throw std::runtime_error("Failed to create cairo surface");
        }
        auto cleanupCairoSurface = qScopeGuard([&, this] { mLibCairo->cairo_surface_destroy_f(cairoSurface); });

        auto cairoCanvas = mLibCairo->cairo_create_f(cairoSurface);
        if (!cairoCanvas) {
            throw std::runtime_error("Failed to create cairo canvas");
        }
        auto cleanupCairoCanvas = qScopeGuard([&, this] { mLibCairo->cairo_destroy_f(cairoCanvas); });

        mLibCairo->cairo_translate_f(cairoCanvas, 0.0, dims.height / 2.0);
        mLibCairo->cairo_scale_f(cairoCanvas, 1.0, -1.0);
        mLibCairo->cairo_translate_f(cairoCanvas, 0.0, -dims.height / 2.0);

        RsvgRectangle viewport{};
        viewport.x = 0;
        viewport.y = 0;
        viewport.width  = dims.width;
        viewport.height = dims.height;

        auto renderStatus = mLibRsvg->rsvg_handle_render_document_f(rsvgHandle, cairoCanvas, &viewport, &rsvgError);
        if (0 != mLibCairo->cairo_surface_status_f(cairoSurface) && !static_cast<bool>(renderStatus)) {
            throw std::runtime_error("Failed to render");
        }

        if (!static_cast<bool>(mLibRsvg->rsvg_handle_close_f(rsvgHandle, &rsvgError))) {
            qDebug() << "Failed to close rsvg handle";
        }

        SwapRedBlue32(image.get());

        //const auto t1 = std::chrono::steady_clock::now();
        //std::cout << "Cairo SVG load time = " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << std::endl;

        const QString libcairoStamp = "libcairo " + mLibCairo->version.toQString();
        const QString librsvgStamp  = "librsvg  " + mLibRsvg->version.toQString();
        FreeImageExt_SetMetadataValue(FIMD_CUSTOM, image.get(), "Rendered by", (libcairoStamp + " & " + librsvgStamp).toStdString());

        return image.release();
    }
    catch (std::exception& err) {
        qDebug() << "PluginSvgCairo[LoadProc]: Error. " << err.what();
    }
    catch (...) {
        qDebug() << "PluginSvgCairo[LoadProc]: Unknown error.";
    }
    return nullptr;
}
