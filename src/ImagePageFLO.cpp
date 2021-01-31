/**
 * @file
 *
 * Copyright 2018-2021 Alexey Gruzdev
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

#include "ImagePageFLO.h"
#include <cassert>
#include <stdexcept>
#include "PluginFLO.h"

namespace
{
    FIBITMAP* convertFloChecked(FIBITMAP* flo)
    {
        FIBITMAP* res = cvtFloToRgb(flo);
        if (!res) {
            throw std::runtime_error("ImagePageFLO[Convert]: Failed to convert FLO to RGB");
        }
        return res;
    }
}


ImagePageFLO::ImagePageFLO(FIBITMAP* flo, FREE_IMAGE_FORMAT fif)
    : ImagePage(convertFloChecked(flo), fif)
    , mFlowImage(flo)
{
    assert(mFlowImage != nullptr);
}

ImagePageFLO::~ImagePageFLO()
{
    FreeImage_Unload(getBitmap());
}

QString ImagePageFLO::doDescribeFormat() const
{
    return "Motion vector 2D Float32";
}

bool ImagePageFLO::doGetPixel(uint32_t y, uint32_t x, Pixel* pixel) const
{
    const DWORD width  = FreeImage_GetWidth(mFlowImage) / 2;
    const DWORD height = FreeImage_GetHeight(mFlowImage);

    if (y < height && x < width) {
        const auto flowLine = reinterpret_cast<const float*>(FreeImage_GetScanLine(mFlowImage, y));
        const DWORD x2 = x << 1;
        pixel->repr = QString("%1, %2").arg(numberToQString(flowLine[x2])).arg(numberToQString(flowLine[x2 + 1]));
        return true;
    }

    return false;
}
