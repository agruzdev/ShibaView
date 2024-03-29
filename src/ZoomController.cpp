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

#include "ZoomController.h"

#include <algorithm>
#include <cassert>
#include <cmath>

static const double kZoomKoef = std::pow(2.0, 1.0 / 7.0);

ZoomController::ZoomController(int32_t baseValue, int32_t fitValue, int32_t scaleValue)
    : mBaseValue(baseValue)
    , mScale(scaleValue)
    , mMinScale(-100)
    , mMaxScale( 100)
{
    assert(mBaseValue > 0);
    if(mBaseValue < 1) {
        mBaseValue = 1;
    }

    setFitValue(fitValue);

    // Avoid value overflow
    mMinScale = std::max(mMinScale, -static_cast<int32_t>(std::floor(std::log(mBaseValue) / std::log(kZoomKoef))));
    mMaxScale = std::min(mMaxScale,  static_cast<int32_t>(std::floor((std::log(std::numeric_limits<int32_t>::max()) - std::log(mBaseValue)) / std::log(kZoomKoef))));
}

ZoomController::~ZoomController() = default;

void ZoomController::setFitValue(int32_t value)
{
    const double fittedScale = (std::log(value) - std::log(mBaseValue)) / std::log(kZoomKoef);
    const double eps = 1.0 / mBaseValue;

    const int32_t oldFloor = mFitScaleFloor;
    const int32_t oldCeil  = mFitScaleCeil;

    mFitScaleFloor = static_cast<int32_t>(std::floor(fittedScale - eps));
    mFitScaleCeil  = static_cast<int32_t>(std::ceil(fittedScale + eps));

    mFitFactor  = static_cast<float>(fittedScale);

    if (mAtFitValue) {
        if (mFitScaleFloor != oldFloor || mFitScaleCeil != oldCeil || mScale < mFitScaleFloor || mScale > mFitScaleCeil) {
            mAtFitValue = false;
        }
    }
    mFitValue = value;
}

void ZoomController::rebase(int32_t baseValue, int32_t fitValue)
{
    if (mAtFitValue) {
        mScale = static_cast<int32_t>(std::round(mFitFactor));
    }
    mBaseValue = baseValue;
    setFitValue(fitValue);
}

float ZoomController::getFactor() const
{
    if (mAtFitValue) {
        return static_cast<float>(std::pow(kZoomKoef, mFitFactor));
    }
    return static_cast<float>(std::pow(kZoomKoef, mScale));
}

int32_t ZoomController::getValue() const
{
    if (mAtFitValue) {
        return mFitValue;
    }
    return static_cast<int>(std::round(mBaseValue * std::pow(kZoomKoef, mScale)));
}

void ZoomController::zoomPlus()
{
    if (mAtFitValue) {
        mScale = mFitScaleCeil;
        mAtFitValue = false;
    }
    else {
        if (mScale == mFitScaleFloor) {
            mAtFitValue = true;
        }
        else {
            mScale = std::min(mScale + 1, mMaxScale);
        }
    }
}

void ZoomController::zoomMinus()
{
    if (mAtFitValue) {
        mScale = mFitScaleFloor;
        mAtFitValue = false;
    }
    else {
        if(mScale == mFitScaleCeil) {
            mAtFitValue = true;
        }
        else {
            mScale = std::max(mScale - 1, mMinScale);
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
    mScale = static_cast<int32_t>(std::round(mFitFactor));
    mAtFitValue = true;
}

