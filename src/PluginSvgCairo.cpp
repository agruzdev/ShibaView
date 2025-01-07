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

#include <Windows.h>
#include <iostream>
#include <filesystem>

#include "FreeImageExt.h"


namespace
{
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


    HMODULE loadLib(const QString& libPath) {
        QFileInfo libPathInfo{ libPath };
        SetDllDirectory(libPathInfo.absolutePath().toStdWString().c_str());
        return LoadLibrary(libPathInfo.absoluteFilePath().toStdWString().c_str());
    }
}


struct PluginSvgCairo::LibRsvg
{
    HMODULE handle{};
    LibVersion version{};

    void* (*rsvg_handle_new_from_data_f)(const char*, unsigned long, GError**) { nullptr };
    void  (*rsvg_handle_get_dimensions_f)(void*, RsvgDimensionData*) { nullptr };
    int   (*rsvg_handle_render_document_f)(void*, void*, const RsvgRectangle*, GError**) { nullptr };


    LibRsvg(const QString& libRsvgPath)
    {
        handle = loadLib(libRsvgPath);
        if (!handle) {
            throw std::runtime_error("PluginSvgCairo[LibRsvg]: Failed to load librsvg");
        }
        if (auto rsvg_major_version = reinterpret_cast<uint32_t*>(GetProcAddress(handle, "rsvg_major_version"))) {
            version.major = *rsvg_major_version;
        }
        if (auto rsvg_minor_version = reinterpret_cast<uint32_t*>(GetProcAddress(handle, "rsvg_minor_version"))) {
            version.minor = *rsvg_minor_version;
        }
        if (auto rsvg_micro_version = reinterpret_cast<uint32_t*>(GetProcAddress(handle, "rsvg_micro_version"))) {
            version.minor = *rsvg_micro_version;
        }

        // https://www.manpagez.com/html/rsvg-2.0/rsvg-2.0-2.42.2/rsvg-RsvgHandle.php#rsvg-handle-new-from-file
        rsvg_handle_new_from_data_f = reinterpret_cast<decltype(rsvg_handle_new_from_data_f)>(GetProcAddress(handle, "rsvg_handle_new_from_data"));
        if (!rsvg_handle_new_from_data_f) {
            throw std::runtime_error("PluginSvgCairo[LibRsvg]: Failed to load the symbol 'rsvg_handle_new_from_data'");
        }

        // https://www.manpagez.com/html/rsvg-2.0/rsvg-2.0-2.40.20/RsvgHandle.php#rsvg-handle-get-dimensions
        rsvg_handle_get_dimensions_f = reinterpret_cast<decltype(rsvg_handle_get_dimensions_f)>(GetProcAddress(handle, "rsvg_handle_get_dimensions"));
        if (!rsvg_handle_get_dimensions_f) {
            throw std::runtime_error("PluginSvgCairo[LibRsvg]: Failed to load the symbol 'rsvg_handle_get_dimensions_f'");
        }

        // https://gnome.pages.gitlab.gnome.org/librsvg/Rsvg-2.0/method.Handle.render_document.html
        rsvg_handle_render_document_f = reinterpret_cast<decltype(rsvg_handle_render_document_f)>(GetProcAddress(handle, "rsvg_handle_render_document"));
        if (!rsvg_handle_render_document_f) {
            throw std::runtime_error("PluginSvgCairo[LibRsvg]: Failed to load the symbol 'rsvg_handle_render_document'");
        }
    }

    ~LibRsvg()
    {
        FreeLibrary(handle);
    }
};

struct PluginSvgCairo::LibCairo
{
    HMODULE handle{};
    LibVersion version{};

    void* (*cairo_image_surface_create_for_data_f)(void*, int, int, int, int) { nullptr };
    int   (*cairo_surface_status_f)(void*) { nullptr };
    void* (*cairo_create_f)(void*) { nullptr };
    void  (*cairo_translate_f)(void*, double, double) { nullptr };
    void  (*cairo_scale_f)(void*, double, double) { nullptr };

    LibCairo(const QString& libCairoPath)
    {
        handle = loadLib(libCairoPath);
        if (!handle) {
            throw std::runtime_error("PluginSvgCairo[LibCairo]: Failed to load libcairo");
        }

        if (auto cairo_version = reinterpret_cast<int(*)()>(GetProcAddress(handle, "cairo_version"))) {
            int cairoVersionValue = cairo_version();
            version.micro = cairoVersionValue % 100;
            version.minor = cairoVersionValue / 100 % 10000;
            version.major = cairoVersionValue / 10000;
        }

        // https://www.cairographics.org/manual/cairo-Image-Surfaces.html#cairo_image_surface_create_for_data
        cairo_image_surface_create_for_data_f = reinterpret_cast<decltype(cairo_image_surface_create_for_data_f)>(GetProcAddress(handle, "cairo_image_surface_create_for_data"));
        if (!cairo_image_surface_create_for_data_f) {
            throw std::runtime_error("PluginSvgCairo[LibCairo]: Failed to load the symbol 'cairo_image_surface_create_for_data_f'");
        }

        // https://www.cairographics.org/manual/cairo-Image-Surfaces.html#cairo-image-surface-create
        // https://github.com/ImageMagick/cairo/blob/5633024ccf6a34b6083cba0a309955a91c619dff/src/cairo.h#L441
        cairo_surface_status_f = reinterpret_cast<decltype(cairo_surface_status_f)>(GetProcAddress(handle, "cairo_surface_status"));
        if (!cairo_surface_status_f) {
            throw std::runtime_error("PluginSvgCairo[LibCairo]: Failed to load the symbol 'cairo_surface_status_f'");
        }

        // https://www.cairographics.org/manual/cairo-cairo-t.html#cairo-create
        cairo_create_f = reinterpret_cast<decltype(cairo_create_f)>(GetProcAddress(handle, "cairo_create"));
        if (!cairo_create_f) {
            throw std::runtime_error("PluginSvgCairo[LibCairo]: Failed to load the symbol 'cairo_create_f'");
        }

        // https://www.cairographics.org/manual/cairo-Transformations.html
        cairo_translate_f = reinterpret_cast<decltype(cairo_translate_f)>(GetProcAddress(handle, "cairo_translate"));
        if (!cairo_translate_f) {
            throw std::runtime_error("PluginSvgCairo[LibCairo]: Failed to load the symbol 'cairo_translate_f'");
        }
        cairo_scale_f = reinterpret_cast<decltype(cairo_scale_f)>(GetProcAddress(handle, "cairo_scale"));
        if (!cairo_scale_f) {
            throw std::runtime_error("PluginSvgCairo[LibCairo]: Failed to load the symbol 'cairo_scale_f'");
        }
    }

    ~LibCairo()
    {
        FreeLibrary(handle);
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
            qDebug() << "Failed to read xml buffer";
            return nullptr;
        }

        GError* rsvg_error = nullptr;
        auto rsvg_handle = mLibRsvg->rsvg_handle_new_from_data_f(xmlBuffer->data(), xmlBuffer->size(), &rsvg_error);

        RsvgDimensionData svg_dims{};
        mLibRsvg->rsvg_handle_get_dimensions_f(rsvg_handle, &svg_dims);
        if (svg_dims.width * svg_dims.height <= 0) {
            svg_dims.width  = 1024;
            svg_dims.height = 1024;
        }

        std::unique_ptr<FIBITMAP, decltype(&::FreeImage_Unload)> image(FreeImage_Allocate(svg_dims.width, svg_dims.height, 32), &::FreeImage_Unload);

        auto cairo_surface = mLibCairo->cairo_image_surface_create_for_data_f(FreeImage_GetBits(image.get()), 0, svg_dims.width, svg_dims.height, FreeImage_GetPitch(image.get()));
        if (0 != mLibCairo->cairo_surface_status_f(cairo_surface)) {
            qDebug() << "Failed to create cairo surface";
            return nullptr;
        }

        auto cairo_canvas = mLibCairo->cairo_create_f(cairo_surface);
        mLibCairo->cairo_translate_f(cairo_canvas, 0.0, svg_dims.height / 2.0);
        mLibCairo->cairo_scale_f(cairo_canvas, 1.0, -1.0);
        mLibCairo->cairo_translate_f(cairo_canvas, 0.0, -svg_dims.height / 2.0);

        RsvgRectangle viewport{};
        viewport.x = 0;
        viewport.y = 0;
        viewport.width  = svg_dims.width;
        viewport.height = svg_dims.height;

        auto render_status = mLibRsvg->rsvg_handle_render_document_f(rsvg_handle, cairo_canvas, &viewport, &rsvg_error);
        if (0 != mLibCairo->cairo_surface_status_f(cairo_surface)) {
            qDebug() << "Failed to render";
            return nullptr;
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
        qDebug() << "Failed to render svg";
    }
    catch (...) {
    }
    return nullptr;
}
