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

#include "ImagePage.h"
#include <cassert>

ImagePage::ImagePage(FIBITMAP* bmp, FREE_IMAGE_FORMAT fif)
    : mBitmap(bmp), mImageFormat(fif)
{
    assert(mBitmap != nullptr);
}

ImagePage::~ImagePage()
{
}

QString ImagePage::doDescribeFormat() const
{
    return FreeImageExt_DescribeImageType(mBitmap);
}

bool ImagePage::doGetPixel(uint32_t y, uint32_t x, Pixel* pixel) const
{
    return Pixel::getBitmapPixel(mBitmap, y, x, pixel);
}

Exif ImagePage::doGetExif() const
{
    return Exif::load(mBitmap);
}
