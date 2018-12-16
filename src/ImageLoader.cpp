/**
* ShibaView
*
* The MIT License (MIT)
* Copyright (c) 2018 Alexey Gruzdev
*/

#include "ImageLoader.h"

#include <QImage>
#include <QFileInfo>

#include <iostream>
#include <memory>

#include "FreeImage.h"

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

    QVector<QRgb> cvtPalette(const RGBQUAD* palette)
    {
        static constexpr uint32_t kPaletteSize = 256;
        QVector<QRgb> qpalette(kPaletteSize);
        for (uint32_t i = 0; i < kPaletteSize; ++i) {
            qpalette[i] = qRgb(palette[i].rgbRed, palette[i].rgbGreen, palette[i].rgbBlue);
        }
        return qpalette;
    }
}

ImageLoader::ImageLoader()
    : QObject(nullptr)
{
    qRegisterMetaType<ImageInfo>("ImageInfo");
}

ImageLoader::~ImageLoader()
{ }

void ImageLoader::onRun(const QString & path)
{
    try {
        const auto upath = path.toStdWString();
#ifdef _MSC_VER
        std::wcout << upath.c_str() << std::endl;
#endif

        std::unique_ptr<FIBITMAP, decltype(&::FreeImage_Unload)> pImg(FreeImage_GenericLoadU(upath.c_str()), &::FreeImage_Unload);
        FIBITMAP* const img = pImg.get();

        if(img == nullptr){
            throw std::runtime_error("Failed to open image");
        }

        FreeImage_FlipVertical(img);

        const uint32_t width  = FreeImage_GetWidth(img);
        const uint32_t height = FreeImage_GetHeight(img);
        const uint32_t bpp    = FreeImage_GetBPP(img);

        std::unique_ptr<FIBITMAP, decltype(&::FreeImage_Unload)> pTmp(nullptr, &::FreeImage_Unload);

        QString formatInfo = "None";

        std::unique_ptr<QImage> qimage;
        switch(FreeImage_GetImageType(img)){
        case FIT_RGBAF:
        case FIT_RGBF: {
            pTmp.reset(FreeImage_ToneMapping(img, FITMO_DRAGO03));
            if(pTmp) {
                qimage = std::make_unique<QImage>(FreeImage_GetBits(pTmp.get()), width, height, FreeImage_GetPitch(pTmp.get()), QImage::Format_RGB888);
                formatInfo = "RGB HDR (tonemapped)";
                break;
            }
            throw std::runtime_error("Unsupported image format " + std::to_string(FreeImage_GetImageType(img)));
        }
        case FIT_RGBA16: {
            assert(bpp == 64);
            qimage = std::make_unique<QImage>(FreeImage_GetBits(img), width, height, FreeImage_GetPitch(img), QImage::Format_RGBA64);
            formatInfo = "RGBA16";
            break;
        }
        case FIT_RGB16: {
            assert(bpp == 48);
            pTmp.reset(FreeImage_ConvertToRGBA16(img));
            if(pTmp) {
                qimage = std::make_unique<QImage>(FreeImage_GetBits(pTmp.get()), width, height, FreeImage_GetPitch(pTmp.get()), QImage::Format_RGBA64);
                formatInfo = "RGB16";
                break;
            }
            throw std::runtime_error("Unsupported image format " + std::to_string(FreeImage_GetImageType(img)));
        }
        case FIT_BITMAP:
            if (32 == bpp) {
                qimage = std::make_unique<QImage>(FreeImage_GetBits(img), width, height, FreeImage_GetPitch(img), QImage::Format_RGBA8888);
                formatInfo = "RGBA8888";
                break;
            }
            else if (24 == bpp) {
                qimage = std::make_unique<QImage>(FreeImage_GetBits(img), width, height, FreeImage_GetPitch(img), QImage::Format_RGB888);
                formatInfo = "RGB888";
                break;
            }
            else if (8 == bpp) {
                const RGBQUAD* palette = FreeImage_GetPalette(img);
                if(palette != nullptr) {
                    qimage = std::make_unique<QImage>(FreeImage_GetBits(img), width, height, FreeImage_GetPitch(img), QImage::Format_Indexed8);
                    qimage->setColorTable(cvtPalette(palette));
                    formatInfo = "RGB Indexed 8bit";
                }
                else {
                    qimage = std::make_unique<QImage>(FreeImage_GetBits(img), width, height, FreeImage_GetPitch(img), QImage::Format_Grayscale8);
                    formatInfo = "Greyscale 8bit";
                }
                break;
            }
            else if(4 == bpp) {
                const RGBQUAD* palette = FreeImage_GetPalette(img);
                if(palette != nullptr) {
                    pTmp.reset(FreeImage_ConvertTo8Bits(img));
                    if(pTmp) {
                        qimage = std::make_unique<QImage>(FreeImage_GetBits(pTmp.get()), width, height, FreeImage_GetPitch(pTmp.get()), QImage::Format_Indexed8);
                        qimage->setColorTable(cvtPalette(palette));
                        formatInfo = "RGB Indexed 4bit";
                        break;
                    }
                }
            }
            else if(1 == bpp) {
                qimage = std::make_unique<QImage>(FreeImage_GetBits(img), width, height, FreeImage_GetPitch(img), QImage::Format_Mono);
                formatInfo = "Mono 1bit";
                break;
            }
            // fallthrough
        default:
            throw std::runtime_error("Unsupported image format " + std::to_string(FreeImage_GetImageType(img)));
        }

        QFileInfo file(path);

        ImageInfo info;
        info.name     = file.fileName();
        info.bytes    = file.size();
        info.format   = formatInfo;
        info.modified = file.lastModified();
        info.dims     = QSize(width, height);

        emit eventResult(QPixmap::fromImage(*qimage), info);
    }
    catch(std::exception & e) {
        emit eventError(QString(e.what()));
    }
    catch(...) {
        emit eventError("Unknown error!");
    }
    deleteLater();
}

