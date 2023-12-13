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

#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include <vector>
#include <variant>
#include <memory>
#include "FreeImage.h"

struct Histogram
{
    std::vector<uint32_t> rgbl;
    double minValue{ };
    double maxValue{ };
    size_t mPixelsNumber{ };

    Histogram(uint32_t maxBinsNumber)
    {
        rgbl.resize(4 * maxBinsNumber);
    }

    void Reset()
    {
        std::memset(rgbl.data(), 0, rgbl.size() * sizeof(uint32_t));
        minValue = maxValue = 0.0;
        mPixelsNumber = 0;
    }

    bool Empty() const
    {
        return (mPixelsNumber == 0);
    }

    size_t GetPixelsNumber() const
    {
        return mPixelsNumber;
    }

    bool FillFromBitmap(FIBITMAP* bmp);

    uint32_t GetMaxBinValue() const
    {
        const auto it = std::max_element(rgbl.cbegin(), rgbl.cend());
        return (it != rgbl.cend()) ? *it : 0u;
    }
};


#endif // HISTOGRAM_H
