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

enum class FrameFlags
    : uint32_t
{
    eNone = 0,
    eRGB = 1,
    eHRD = 2,
};

inline
FrameFlags operator|(FrameFlags f1, FrameFlags f2)
{
    return static_cast<FrameFlags>(static_cast<std::underlying_type_t<FrameFlags>>(f1) | static_cast<std::underlying_type_t<FrameFlags>>(f2));
}

inline
FrameFlags operator&(FrameFlags f1, FrameFlags f2)
{
    return static_cast<FrameFlags>(static_cast<std::underlying_type_t<FrameFlags>>(f1) & static_cast<std::underlying_type_t<FrameFlags>>(f2));
}

inline
bool testFlag(FrameFlags flags, FrameFlags test)
{
    return 0 != (static_cast<std::underlying_type_t<FrameFlags>>(flags) & static_cast<std::underlying_type_t<FrameFlags>>(test));
}

static Q_CONSTEXPR uint32_t kNoneIndex = std::numeric_limits<uint32_t>::max();

class ImagePage
{
public:
    ImagePage(FIBITMAP* bmp, uint32_t index);

    ImagePage(const ImagePage&) = delete;

    ImagePage(ImagePage&&) = delete;

    virtual ~ImagePage();

    ImagePage& operator=(const ImagePage&) = delete;

    ImagePage& operator=(ImagePage&&) = delete;

    QString describeFormat() const
    {
        return doDescribeFormat();
    }

    /**
     * Original image source bitmap
     */
    FIBITMAP* getSourceBitmap() const
    {
        return mBitmap;
    }

    /**
     * Converted internal representation
     */
    FIBITMAP* getBitmap() const
    {
        return mConvertedBitmap;
    }

    uint32_t index() const
    {
        return mIndex;
    }

    void setAnimation(AnimationInfo anim)
    {
        mAnimation = std::move(anim);
    }

    const AnimationInfo& animation() const
    {
        return mAnimation;
    }

    bool getPixel(uint32_t y, uint32_t x, Pixel* pixel) const
    {
        return doGetPixel(y, x, pixel);
    }

    const FrameFlags& flags() const
    {
        return mFlags;
    }

    const Exif& getExif() const;

    size_t getMemorySize() const;

    UniqueBitmap getOrMakeThumbnail(uint32_t maxSize) const;

protected:
    virtual QString doDescribeFormat() const;

    virtual Exif doGetExif() const;

    virtual bool doGetPixel(uint32_t y, uint32_t x, Pixel* pixel) const;

private:
    FIBITMAP* mBitmap;
    uint32_t mIndex;
    FIBITMAP* mConvertedBitmap;
    bool mFrameNeedsUnload = false;
    FrameFlags mFlags;
    AnimationInfo mAnimation;
    mutable std::unique_ptr<Exif> mExif;
};



#endif // IMAGEPAGE_H
