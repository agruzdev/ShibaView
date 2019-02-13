/**
* ShibaView
*
* The MIT License (MIT)
* Copyright (c) 2018 Alexey Gruzdev
*/

#include "ImageSource.h"

#include <QDebug>

#include "BitmapSource.h"
#include "MultiBitmapsource.h"


std::unique_ptr<ImageSource> ImageSource::Load(const QString & filename) Q_DECL_NOEXCEPT
{
    std::unique_ptr<ImageSource> source = nullptr;
#ifdef _WIN32
    const auto uniName = filename.toStdWString();
    FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeU(uniName.c_str(), 0);
    if (fif == FIF_UNKNOWN) {
        fif = FreeImage_GetFIFFromFilenameU(uniName.c_str());
    }
#else
    const auto utfName = filename.toUtf8().toStdString();
    FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(utfName.c_str(), 0);
    if (fif == FIF_UNKNOWN) {
        fif = FreeImage_GetFIFFromFilename(utfName.c_str());
    }
#endif
    if ((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif)) {
        try {
            switch(fif) {
            case FIF_GIF:
            case FIF_ICO:
            case FIF_TIFF:
                source = std::make_unique<MultibitmapSource>(filename, fif);
                break;
            default:
                source = std::make_unique<BitmapSource>(filename, fif);
                break;
            }
        }
        catch(std::exception & err) {
            qDebug() << err.what();
        }
        catch(...) {
            qDebug() << "ImageSource[Load]: Unknown error";
        }
    }
    return source;
}
