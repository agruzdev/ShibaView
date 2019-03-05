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

#include <cstdint>

class ZoomController
{
public:
    ZoomController(int baseValue, int fitValue);
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
    int getValue() const;

    /**
     * Update fit piont
     */
    void setFitValue(int value);

    /**
     * Reset for a new base value preserving current zoom level
     */
    void rebase(int baseValue, int fitValue);

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

private:
    int32_t mBaseValue;

    int32_t mScale;
    int32_t mMinScale;
    int32_t mMaxScale;

    int32_t mFitValue;
    int32_t mFitScaleFloor;
    int32_t mFitScaleCeil;

    bool mAtFitValue;

    float mFitOffset; // offset inside grid
};

#endif // ZOOMCONTROLLER_H
