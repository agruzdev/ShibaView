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

#include "ZoomController.h"

#include <algorithm>
#include <cmath>

ZoomController::ZoomController(int pos100, int posFit, int minValue, int maxValue)
{
    const float step = 0.15f;

    const int node1 = std::min(pos100, posFit);
    const int node2 = std::max(pos100, posFit);

    mScales.push_back(node1);
    int offset1 = 0;
    float v = static_cast<float>(node1);
    for(;;) {
        v = v * (1.0f - step);
        const int s = std::max(static_cast<int>(std::floor(v)), minValue);
        if(s != mScales.front()) {
            mScales.push_front(s);
            ++offset1;
        }
        if (s == minValue) {
            break;
        }
    }

    v = static_cast<float>(node1);
    int offset2 = offset1;
    for(;;) {
        v = v * (1.0f + step);
        const int s = std::min(static_cast<int>(std::ceil(v)), node2);
        if(s != mScales.back()) {
            mScales.push_back(s);
            ++offset2;
        }
        if (s == node2) {
            break;
        }
    }

    v = static_cast<float>(node2);
    for(;;) {
        v = v * (1.0f + step);
        const int s = std::min(static_cast<int>(std::ceil(v)), maxValue);
        if(s != mScales.back()) {
            mScales.push_back(s);
        }
        if (s == maxValue) {
            break;
        }
    }

    if(pos100 < posFit) {
        mPos100 = std::next(mScales.cbegin(), offset1);
        mPosFit = std::next(mScales.cbegin(), offset2);
    }
    else {
        mPosFit = std::next(mScales.cbegin(), offset1);
        mPos100 = std::next(mScales.cbegin(), offset2);
    }
    mPosCurrent = mPosFit;
}

ZoomController::~ZoomController()
{
    
}
