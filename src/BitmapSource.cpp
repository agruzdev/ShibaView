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

#include "BitmapSource.h"

BitmapSource::BitmapSource(const QString & filename, FREE_IMAGE_FORMAT fif)
{
#ifdef _WIN32
    const auto uniName = filename.toStdWString();
    mBitmap = FreeImage_LoadU(fif, uniName.c_str(), JPEG_EXIFROTATE);
#else
    const auto utfName = filename.toUtf8().toStdString();
    mBitmap = FreeImage_Load(fif, utfName.c_str(), JPEG_EXIFROTATE);
#endif
    if (nullptr == mBitmap) {
        throw std::runtime_error("BitmapSource[BitmapSource]: Failed to load file.");
    }
}

BitmapSource::~BitmapSource()
{
    FreeImage_Unload(mBitmap);
}

uint32_t BitmapSource::pagesCount() const Q_DECL_NOEXCEPT
{
    return 1;
}

FIBITMAP* BitmapSource::decodePage(uint32_t /*page*/, AnimationInfo* /*anim*/) Q_DECL_NOEXCEPT
{
    return mBitmap;
}

void BitmapSource::releasePage(FIBITMAP*) Q_DECL_NOEXCEPT
{
}

bool BitmapSource::storesResidual() const Q_DECL_NOEXCEPT
{
    return false;
}