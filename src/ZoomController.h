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

#ifndef ZOOMCONTROLLER_H
#define ZOOMCONTROLLER_H

#include <cstdint>

class ZoomController
{
public:
    ZoomController(int32_t baseValue, int32_t fitValue, int32_t scaleValue = 0);
    ~ZoomController();

    ZoomController(const ZoomController&) = default;
    ZoomController(ZoomController&&) = default;
    ZoomController& operator=(const ZoomController&) = default;
    ZoomController& operator=(ZoomController&&) = default;

    /**
     * Get zoom factor
     */
    float getFactor() const;

    /**
     * Get zoomed value = baseVal * zoomFactor
     */
    int32_t getValue() const;

    /**Get fit value
     */
    int32_t getFitValue() const
    {
        return mFitValue;
    }

    /**
     * Update fit piont
     */
    void setFitValue(int32_t value);

    /**
     * Reset for a new base value preserving current zoom level
     */
    void rebase(int32_t baseValue, int32_t fitValue);

    /**
     * Reset for a new base value preserving current zoom level
     */
    void rebase(int32_t baseValue)
    {
        rebase(baseValue, mFitValue);
    }

    /**
     * Move zoom position
     */
    void zoomPlus();

    /**
     * Move zoom position
     */
    void zoomMinus();

    /**
     * Reset to initial state (no zoom)
     */
    void moveToIdentity();

    /**
     * Reset to fit value zoom
     */
    void moveToFit();

    /**
     * Get current scale value
     */
    int32_t getScaleValue() const
    {
        return mScale;
    }

private:
    int32_t mBaseValue;

    int32_t mScale;
    int32_t mMinScale;
    int32_t mMaxScale;

    int32_t mFitValue;
    int32_t mFitScaleFloor;
    int32_t mFitScaleCeil;

    bool mAtFitValue;

    float mFitFactor;
};

#endif // ZOOMCONTROLLER_H
