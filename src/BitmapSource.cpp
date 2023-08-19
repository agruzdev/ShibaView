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

#include "BitmapSource.h"
#include <stdexcept>
#include "FreeImageExt.h"
#include "ImagePageFLO.h"

BitmapSource::BitmapSource(const QString & filename, FREE_IMAGE_FORMAT fif)
    : mImageFormat(fif)
{
#ifdef _WIN32
    const auto uniName = filename.toStdWString();
    mBitmap = FreeImage_LoadU(mImageFormat, uniName.c_str(), JPEG_EXIFROTATE);
#else
    const auto utfName = filename.toUtf8().toStdString();
    mBitmap = FreeImage_Load(mImageFormat, utfName.c_str(), JPEG_EXIFROTATE);
#endif
    if (nullptr == mBitmap) {
        throw std::runtime_error("BitmapSource[BitmapSource]: Failed to load file.");
    }
}

BitmapSource::~BitmapSource()
{
    FreeImage_Unload(mBitmap);
}

uint32_t BitmapSource::doPagesCount() const
{
    return 1;
}

const ImagePage* BitmapSource::doDecodePage(uint32_t /*pageIdx*/)
{
    switch(mImageFormat) {
    case FIEF_FLO:
        return new ImagePageFLO(mBitmap, mImageFormat);
    default:
        return new ImagePage(mBitmap, mImageFormat);
    }
}

void BitmapSource::doReleasePage(const ImagePage* page)
{
    delete page;
}

bool BitmapSource::doStoresDifference() const
{
    return false;
}
