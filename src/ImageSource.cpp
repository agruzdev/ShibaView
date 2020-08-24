/**
 * @file
 *
 * Copyright 2018-2019 Alexey Gruzdev
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

#include "ImageSource.h"

#include <QDebug>

#include "BitmapSource.h"
#include "MultiBitmapsource.h"
#include "RawSource.h"


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

    if (!source) {
        // Treat as raw binary blob if nothing worked
        try {
            source = std::make_unique<RawSource>(filename);
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
