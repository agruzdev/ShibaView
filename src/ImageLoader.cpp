/**
* ShibaView
*
* The MIT License (MIT)
* Copyright (c) 2018 Alexey Gruzdev
*/

#include "ImageLoader.h"

#include <QImage>

#include <iostream>
#include <memory>
#include "FreeImage.h"

namespace
{
    // From FreeImage manual

    /** Generic image loader
    @param lpszPathName Pointer to the full file name
    @param flag Optional load flag constant
    @return Returns the loaded dib if successful, returns NULL otherwise
    */
    FIBITMAP* FreeImage_GenericLoadU(const wchar_t* lpszPathName, int flag = 0)
    {
      FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
      // check the file signature and deduce its format
      // (the second argument is currently not used by FreeImage)
      fif = FreeImage_GetFileTypeU(lpszPathName, 0);
      if(fif == FIF_UNKNOWN) {
        // no signature ?
        // try to guess the file format from the file extension
        fif = FreeImage_GetFIFFromFilenameU(lpszPathName);
      }
      // check that the plugin has reading capabilities ...
      if((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif)) {
        // ok, let's load the file
        FIBITMAP *dib = FreeImage_LoadU(fif, lpszPathName, flag);
        // unless a bad file format, we are done !
        return dib;
      }
      return NULL;
    }
}

ImageLoader::ImageLoader()
    : QObject(nullptr)
{ }

ImageLoader::~ImageLoader()
{ }

void ImageLoader::onRun(const QString & path)
{
    QPixmap res{};
    try {
        const auto upath = path.toStdWString();
        std::wcout << upath.c_str() << std::endl;

        std::unique_ptr<FIBITMAP, decltype(&::FreeImage_Unload)> pImg(FreeImage_GenericLoadU(upath.c_str()), &::FreeImage_Unload);
        FIBITMAP* const img = pImg.get();

        if(img == nullptr){
            throw std::runtime_error("Failed to open image");
        }

        FreeImage_FlipVertical(img);

        const uint32_t width  = FreeImage_GetWidth(img);
        const uint32_t height = FreeImage_GetHeight(img);
        const uint32_t bpp    = FreeImage_GetBPP(img);

        std::unique_ptr<QImage> qimage;
        switch(FreeImage_GetImageType(img)){
        case FIT_BITMAP:
            if(32 == bpp) {
                qimage.reset(new QImage(FreeImage_GetBits(img), width, height, FreeImage_GetPitch(img), QImage::Format_RGBA8888));
                break;
            }
            else if(24 == bpp) {
                qimage.reset(new QImage(FreeImage_GetBits(img), width, height, FreeImage_GetPitch(img), QImage::Format_RGB888));
                break;
            }
            else if(8 == bpp) {
                static constexpr uint32_t kPaletteSize = 256;
                const RGBQUAD* palette = FreeImage_GetPalette(img);
                if(palette != nullptr) {
                    qimage.reset(new QImage(FreeImage_GetBits(img), width, height, FreeImage_GetPitch(img), QImage::Format_Indexed8));
                    QVector<QRgb> qpalette(kPaletteSize);
                    for (uint32_t i = 0; i < kPaletteSize; ++i) {
                        qpalette[i] = qRgb(palette[i].rgbRed, palette[i].rgbGreen, palette[i].rgbBlue);
                    }
                    qimage->setColorTable(qpalette);
                }
                else {
                    qimage.reset(new QImage(FreeImage_GetBits(img), width, height, FreeImage_GetPitch(img), QImage::Format_Grayscale8));
                }
                break;
            }
            // fallthrough
        default:
            throw std::runtime_error("Unsupported image format " + std::to_string(FreeImage_GetImageType(img)));
        }
        res = QPixmap::fromImage(*qimage);
    }
    catch(std::exception & e) {
        qCritical(e.what());
    }
    catch(...) {
        qCritical("Unknown exception!");
    }
    emit eventResult(res);
    deleteLater();
}
