#include "Image.h"

#include <iostream>

#include <QDateTime>
#include <QDebug>
#include <QFileInfo>

namespace
{
    // From FreeImage manual

    /** Generic image loader
     * @param lpszPathName Pointer to the full file name
     * @param flag Optional load flag constant
     * @return Returns the loaded dib if successful, returns NULL otherwise
     */
    FIBITMAP* FreeImage_GenericLoadU(const wchar_t* lpszPathName, int flag = 0)
    {
        FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeU(lpszPathName, 0);
        if(fif == FIF_UNKNOWN) {
            fif = FreeImage_GetFIFFromFilenameU(lpszPathName);
        }
        if((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif)) {
            return FreeImage_LoadU(fif, lpszPathName, flag);
        }
        return nullptr;
    }
}



Image::Image(QString name, QString filename) noexcept
{
    const auto upath = filename.toStdWString();
#ifdef _MSC_VER
    std::wcout << upath.c_str() << std::endl;
#endif

    uint32_t width  = 0;
    uint32_t height = 0;
    QString formatInfo = "None";

    // Load bitmap. Keep empty on fail.
    try {
        std::unique_ptr<FIBITMAP, decltype(&::FreeImage_Unload)> img(FreeImage_GenericLoadU(upath.c_str(), JPEG_EXIFROTATE), &::FreeImage_Unload);
        if (!img) {
            throw std::runtime_error("Failed to open image");
        }

        width  = FreeImage_GetWidth(img.get());
        height = FreeImage_GetHeight(img.get());
        const uint32_t bpp    = FreeImage_GetBPP(img.get());

        QImage imageView;
        const auto imgType = FreeImage_GetImageType(img.get());
        switch (imgType) {
        case FIT_RGBAF:
            formatInfo = "RGB HDR (tonemapped)";
            goto ToneMapping;
        case FIT_RGBF:
            formatInfo = "RGB HDR (tonemapped)";
        ToneMapping:
            mBitmap.reset(FreeImage_ToneMapping(img.get(), FITMO_DRAGO03));
            if (mBitmap) {
                imageView = QImage(FreeImage_GetBits(mBitmap.get()), width, height, FreeImage_GetPitch(mBitmap.get()), QImage::Format_RGB888);
            }
            break;

        case FIT_RGBA16:
            assert(bpp == 64);
            mBitmap.reset(FreeImage_ConvertTo32Bits(img.get()));
            if (mBitmap) {
                imageView  = QImage(FreeImage_GetBits(mBitmap.get()), width, height, FreeImage_GetPitch(mBitmap.get()), QImage::Format_RGBA8888);
                formatInfo = "RGBA16";
            }
            break;

        case FIT_RGB16:
            assert(bpp == 48);
            mBitmap.reset(FreeImage_ConvertTo24Bits(img.get()));
            if (mBitmap) {
                imageView  = QImage(FreeImage_GetBits(mBitmap.get()), width, height, FreeImage_GetPitch(mBitmap.get()), QImage::Format_RGB888);
                formatInfo = "RGB16";
            }
            break;

        case FIT_UINT16:
            formatInfo = "Greyscale 16bit";
            goto ConvertToStandardType;
        case FIT_INT16:
            formatInfo = "Greyscale 16bit (signed)";
            goto ConvertToStandardType;
        case FIT_UINT32:
            formatInfo = "Greyscale 32bit";
            goto ConvertToStandardType;
        case FIT_INT32:
            formatInfo = "Greyscale 32bit (signed)";
            goto ConvertToStandardType;
        case FIT_FLOAT:
            formatInfo = "Greyscale float";
            goto ConvertToStandardType;
        case FIT_DOUBLE:
            formatInfo = "Greyscale double";

        ConvertToStandardType:
            mBitmap.reset(FreeImage_ConvertToStandardType(img.get()));
            if (mBitmap) {
                imageView = QImage(FreeImage_GetBits(mBitmap.get()), width, height, FreeImage_GetPitch(mBitmap.get()), QImage::Format_Grayscale8);
            }
            break;

        case FIT_BITMAP:
            if (32 == bpp) {
                mBitmap    = std::move(img);
                imageView  = QImage(FreeImage_GetBits(mBitmap.get()), width, height, FreeImage_GetPitch(mBitmap.get()), QImage::Format_RGBA8888);
                formatInfo = "RGBA8888";
            }
            else if (24 == bpp) {
                mBitmap    = std::move(img);
                imageView  = QImage(FreeImage_GetBits(mBitmap.get()), width, height, FreeImage_GetPitch(mBitmap.get()), QImage::Format_RGB888);
                formatInfo = "RGB888";
            }
            else if (8 == bpp) {
                if (FIC_PALETTE == FreeImage_GetColorType(img.get())) {
                    mBitmap.reset(FreeImage_ConvertTo24Bits(img.get()));
                    imageView  = QImage(FreeImage_GetBits(mBitmap.get()), width, height, FreeImage_GetPitch(mBitmap.get()), QImage::Format_RGB888);
                    formatInfo = "RGB Indexed 8bit";
                }
                else {
                    mBitmap    = std::move(img);
                    imageView  = QImage(FreeImage_GetBits(mBitmap.get()), width, height, FreeImage_GetPitch(mBitmap.get()), QImage::Format_Grayscale8);
                    formatInfo = "Greyscale 8bit";
                }
            }
            else if(4 == bpp) {
                mBitmap.reset(FreeImage_ConvertTo24Bits(img.get()));
                imageView  = QImage(FreeImage_GetBits(mBitmap.get()), width, height, FreeImage_GetPitch(mBitmap.get()), QImage::Format_RGB888);
                formatInfo = "RGB Indexed 4bit";
            }
            else if(1 == bpp) {
                mBitmap    = std::move(img);
                imageView  = QImage(FreeImage_GetBits(mBitmap.get()), width, height, FreeImage_GetPitch(mBitmap.get()), QImage::Format_Mono);
                formatInfo = "Mono 1bit";
            }
            break;

        default:
            break;
        }

        if (!mBitmap || imageView.isNull()) {
            throw std::runtime_error("Unsupported image format " + std::to_string(imgType));
        }

        FreeImage_FlipVertical(mBitmap.get());
        mPixmap = QPixmap::fromImage(imageView);
    }
    catch(std::exception & e) {
        qWarning() << QString(e.what());
        mBitmap.reset();
        mPixmap = QPixmap();
    }
    catch(...) {
        qWarning() << QString("Unknown error!");
        mBitmap.reset();
        mPixmap = QPixmap();
    }

    QFileInfo file(filename);

    mInfo.path     = std::move(filename);
    mInfo.name     = std::move(name);
    mInfo.bytes    = file.size();
    mInfo.format   = formatInfo;
    mInfo.modified = file.lastModified();
    mInfo.dims     = QSize(width, height);
}

Image::~Image() = default;

uint32_t Image::width() const
{
    if (mRotation == Rotation::eDegree90 || mRotation == Rotation::eDegree270) {
        return sourceHeight();
    }
    else {
        return sourceWidth();
    }
}

uint32_t Image::height() const
{
    if (mRotation == Rotation::eDegree90 || mRotation == Rotation::eDegree270) {
        return sourceWidth();
    }
    else {
        return sourceHeight();
    }
}

uint32_t Image::sourceWidth() const
{
    return FreeImage_GetWidth(mBitmap.get());
}

uint32_t Image::sourceHeight() const
{
    return FreeImage_GetHeight(mBitmap.get());
}

const QPixmap & Image::pixmap() const
{
    if(isValid() && mInvalidTransform) {
        mPixmap = RecalculatePixmap();
        mInvalidTransform = false;
    }
    return mPixmap;
}

QPixmap Image::RecalculatePixmap() const
{
    assert(mBitmap != nullptr);

    std::unique_ptr<FIBITMAP, decltype(&::FreeImage_Unload)> rotated(nullptr, &::FreeImage_Unload);
    FIBITMAP* target  = mBitmap.get();
    if (mRotation != Rotation::eDegree0) {
        rotated.reset(FreeImage_Rotate(target, static_cast<double>(mRotation)));
        target = rotated.get();
    }

    QImage imageView;

    assert(target != nullptr);
    const uint32_t width  = FreeImage_GetWidth(target);
    const uint32_t height = FreeImage_GetHeight(target);
    const uint32_t bpp    = FreeImage_GetBPP(target);
    switch (bpp) {
    case 1:
        imageView = QImage(FreeImage_GetBits(target), width, height, FreeImage_GetPitch(target), QImage::Format_Mono);
        break;
    case 8:
        imageView = QImage(FreeImage_GetBits(target), width, height, FreeImage_GetPitch(target), QImage::Format_Grayscale8);
        break;
    case 24:
        imageView = QImage(FreeImage_GetBits(target), width, height, FreeImage_GetPitch(target), QImage::Format_RGB888);
        break;
    case 32:
        imageView = QImage(FreeImage_GetBits(target), width, height, FreeImage_GetPitch(target), QImage::Format_RGBA8888);
        break;
    default:
        throw std::logic_error("Internal image is 24 or 32 bit");
    }

    return QPixmap::fromImage(imageView);
}
