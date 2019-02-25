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

#ifndef ZOOMCONTROLLER_H
#define ZOOMCONTROLLER_H

#include <deque>

class ZoomController
{
public:
    ZoomController(int pos100, int posFit, int minValue, int maxValue);
    ~ZoomController();

    int get() const
    {
        return *mPosCurrent;
    }

    int zoomPlus()
    {
        if(mPosCurrent != std::prev(mScales.cend())) {
            ++mPosCurrent;
        }
        return *mPosCurrent;
    }

    int zoomMinus()
    {
        if(mPosCurrent != mScales.cbegin()) {
            --mPosCurrent;
        }
        return *mPosCurrent;
    }

    int moveToPos100()
    {
        mPosCurrent = mPos100;
        return *mPosCurrent;
    }

    int moveToPosFit()
    {
        mPosCurrent = mPosFit;
        return *mPosCurrent;
    }

private:
    std::deque<int> mScales;
    std::deque<int>::const_iterator mPos100;
    std::deque<int>::const_iterator mPosFit;
    std::deque<int>::const_iterator mPosCurrent;
};

#endif // ZOOMCONTROLLER_H