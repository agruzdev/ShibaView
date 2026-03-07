/**
 * @file
 *
 * Copyright 2018-2026 Alexey Gruzdev
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

#include <stdexcept>
#include <QDebug>

#include "BitmapSource.h"
#include "MultiBitmapsource.h"
#include "PluginManager.h"


namespace
{
    constexpr
    bool isMultiPage(FREE_IMAGE_FORMAT fif) {
        switch (fif) {
            case FIF_GIF:
            case FIF_ICO:
            case FIF_TIFF:
                return true;
            default:
                return false;
        }
    }

} // namespace


std::shared_ptr<ImageSource> ImageSource::Load(const QString & filename) Q_DECL_NOEXCEPT
{
    std::shared_ptr<ImageSource> source = nullptr;

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
            if (isMultiPage(fif) || fif == PluginManager::getInstance().getSvgId()) {
                source = std::make_shared<MultibitmapSource>(filename, fif);
            }
            else {
                source = std::make_shared<BitmapSource>(filename, fif);
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

void ImageSource::Save(FIBITMAP* bmp, const QString& filename) 
{
#ifdef _WIN32
    const auto uniName = filename.toStdWString();
    const auto fif = FreeImage_GetFIFFromFilenameU(uniName.c_str());
#else
    const auto utfName = filename.toUtf8().toStdString();
    const auto fif = FreeImage_GetFIFFromFilename(utfName.c_str());
#endif
    if (fif == FIF_UNKNOWN) {
        throw std::runtime_error("Unknown file format");
    }

    bool success{ false };
    if (FreeImage_FIFSupportsWriting(fif)) {
#ifdef _WIN32
        success = FreeImage_SaveU(fif, bmp, uniName.c_str());
#else
        success = FreeImage_Save(fif, bmp, utfName.c_str());
#endif
    }

    if (!success) {
        throw std::runtime_error("Failed to write file");
    }
}
