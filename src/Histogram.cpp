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

#include "Histogram.h"

namespace {
    union ValueStorage {
        uint8_t u8;
        uint16_t u16;
        uint32_t u32;
        int32_t i32;
        float f32;
        double f64;
    };
};

bool Histogram::FillFromBitmap(FIBITMAP* bmp)
{
    ValueStorage minValStorage, maxValStorage;
    if (!FreeImage_MakeHistogram(bmp, rgbl.size() / 4, &minValStorage, &maxValStorage, rgbl.data(), 4, rgbl.data() + 1, 4, rgbl.data() + 2, 4, rgbl.data() + 3, 4)) {
        return false;
    }
    if (!CastPixelValue(FreeImage_GetImageType(bmp), &minValStorage, FIT_DOUBLE, &minValue)) {
        return false;
    }
    if (!CastPixelValue(FreeImage_GetImageType(bmp), &maxValStorage, FIT_DOUBLE, &maxValue)) {
        return false;
    }
    return true;
}
