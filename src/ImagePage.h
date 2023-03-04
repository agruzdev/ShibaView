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

#ifndef IMAGEPAGE_H
#define IMAGEPAGE_H

#include <cassert>
#include <cstdint>
#include <memory>

#include <QString>

#include "FreeImageExt.h"
#include "Pixel.h"
#include "Exif.h"

enum class DisposalType
    : uint8_t
{
    eUnspecified = 0,
    eLeave = 1,
    eBackground = 2,
    ePrevious = 3
};

struct AnimationInfo
{
    uint16_t offsetX  = 0;
    uint16_t offsetY  = 0;
    uint32_t duration = 0;
    DisposalType disposal = DisposalType::eUnspecified;
};

class ImagePage
{
public:
    ImagePage(FIBITMAP* bmp, FREE_IMAGE_FORMAT fif);

    ImagePage(const ImagePage&) = delete;

    ImagePage(ImagePage&&) = delete;

    virtual ~ImagePage();

    ImagePage& operator=(const ImagePage&) = delete;

    ImagePage& operator=(ImagePage&&) = delete;

    QString describeFormat() const
    {
        return doDescribeFormat();
    }

    FIBITMAP* getBitmap() const
    {
        return mBitmap;
    }

    const AnimationInfo& getAnimation() const
    {
        return mAnimation;
    }

    void setAnimation(AnimationInfo anim)
    {
        mAnimation = std::move(anim);
    }

    bool getPixel(uint32_t y, uint32_t x, Pixel* pixel) const
    {
        return doGetPixel(y, x, pixel);
    }

    const Exif& getExif() const
    {
        if (!mExif) {
            mExif = std::make_unique<Exif>(doGetExif());
        }
        return *mExif;
    }

protected:
    virtual QString doDescribeFormat() const;

    virtual Exif doGetExif() const;

    virtual bool doGetPixel(uint32_t y, uint32_t x, Pixel* pixel) const;

private:
    FIBITMAP* mBitmap;
    FREE_IMAGE_FORMAT mImageFormat;
    AnimationInfo mAnimation;
    mutable std::unique_ptr<Exif> mExif;
};



#endif // IMAGEPAGE_H
