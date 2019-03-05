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

static constexpr double kZoomKoef = 0.9;

ZoomController::ZoomController(int baseValue, int fitValue)
    : mBaseValue(baseValue)
    , mScale(0)
    , mMinScale(-100)
    , mMaxScale( 100)
{
    setFitValue(fitValue);
}

ZoomController::~ZoomController() = default;

void ZoomController::setFitValue(int valueFit)
{
    mFitValue = valueFit;

    const double fittedScale = (std::log(mFitValue) - std::log(mBaseValue)) / std::log(kZoomKoef);
    const double eps = 1.0 / mBaseValue;

    mFitScaleFloor = static_cast<int32_t>(std::floor(fittedScale - eps));
    mFitScaleCeil  = static_cast<int32_t>(std::ceil (fittedScale + eps));

    mFitOffset  = fittedScale - mFitScaleFloor;

    mAtFitValue = (mFitScaleFloor <= mScale) && (mScale <= mFitScaleCeil);
}

void ZoomController::rebase(int baseValue, int fitValue)
{
    const float factor = getFactor();
    const double newScale = (std::log(baseValue * factor) - std::log(baseValue)) / std::log(kZoomKoef);
    mScale = static_cast<int32_t>(std::round(newScale));
    mBaseValue = baseValue;

    setFitValue(fitValue);
}

float ZoomController::getFactor() const
{
    if (mAtFitValue) {
        return static_cast<float>(std::pow(kZoomKoef, mFitScaleFloor + mFitOffset));
    }
    return static_cast<float>(std::pow(kZoomKoef, mScale));
}

int ZoomController::getValue() const
{
    if (mAtFitValue) {
        return mFitValue;
    }
    return static_cast<int>(std::round(mBaseValue * std::pow(kZoomKoef, mScale)));
}

void ZoomController::zoomPlus()
{
    if (mAtFitValue) {
        mScale = mFitScaleFloor;
        mAtFitValue = false;
    }
    else {
        if (mScale == mFitScaleCeil) {
            mAtFitValue = true;
        }
        else {
            mScale = std::max(mScale - 1, mMinScale);
        }
    }
}

void ZoomController::zoomMinus()
{
    if (mAtFitValue) {
        mScale = mFitScaleCeil;
        mAtFitValue = false;
    }
    else {
        if(mScale == mFitScaleFloor) {
            mAtFitValue = true;
        }
        else {
            mScale = std::min(mScale + 1, mMaxScale);
        }
    }
}

void ZoomController::moveToIdentity()
{
    mScale = 0;
    mAtFitValue = false;
}

void ZoomController::moveToFit()
{
    mAtFitValue = true;
}

