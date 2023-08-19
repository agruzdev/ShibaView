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

#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <QSharedPointer>
#include <QPixmap>

#include "FreeImageExt.h"
#include "EnumArray.h"
#include "Image.h"

enum class Rotation
{
    eDegree0   = 0,
    eDegree90  = 1,
    eDegree180 = 2,
    eDegree270 = 3,

    length_
};

constexpr
int toDegree(Rotation r)
{
    return 90 * static_cast<int>(r);
}

enum class FlipType
{
    eHorizontal,
    eVertical,

    length_
};


enum class ChannelSwizzle
{
    eRGB = 0,
    eBGR,
    eRed,
    eGreen,
    eBlue,
    eAlpha,

    length_
};


class ImageProcessor
    : public ImageListener
{
public:
    ImageProcessor();

    ImageProcessor(QWeakPointer<Image> image)
        : ImageProcessor()
    {
        attachSource(image);
    }

    ImageProcessor(const ImageProcessor&) = delete;

    ImageProcessor(ImageProcessor&&) = delete;

    ~ImageProcessor();

    ImageProcessor& operator=(const ImageProcessor&) = delete;

    ImageProcessor& operator=(ImageProcessor&&) = delete;

    void attachSource(QWeakPointer<Image> image);

    void detachSource();

    Rotation rotation() const
    {
        return mRotation;
    }
    
    void setRotation(Rotation r)
    {
        if (mRotation != r) {
            mRotation = r;
            mIsValid = false;
        }
    }

    void setFlip(FlipType flip, bool value)
    {
        if (mFlips[flip] != value) {
            mFlips[flip] = value;
            mIsValid = false;
        }
    }

    FREE_IMAGE_TMO toneMappingMode() const
    {
        return mToneMapping;
    }

    void setToneMappingMode(FREE_IMAGE_TMO mode)
    {
        if(mToneMapping != mode) {
            mToneMapping = mode;
            mIsValid = false;
        }
    }

    void setGamma(double value)
    {
        if (mGammaValue != value) {
            mGammaValue = value;
            mIsValid = false;
        }
    }

    double getGamma() const
    {
        return mGammaValue;
    }

    /**
     * Setup arbitrary channel order
     */
    void setChannelSwizzle(ChannelSwizzle swizzle)
    {
        if (mSwizzleType != swizzle) {
            mSwizzleType = swizzle;
            mIsValid = false;
        }
    }

    ChannelSwizzle getChannelSwizzle() const
    {
        return mSwizzleType;
    }

    /**
     * Apply ineversed transform to coordinates and call Image::getPixel
     */
    bool getPixel(uint32_t y, uint32_t x, Pixel* p) const;

    /**
     * Image width after processing
     */
    uint32_t width() const;

    /**
     * Image height after processing
     */
    uint32_t height() const;

    /**
     * Processed frame, ready to draw
     */
    const QPixmap& getResultPixmap();

    /**
     * Processed frame, ready to draw
     */
    const UniqueBitmap& getResultBitmap();

private:
    void onInvalidated(Image* emitter) override;

    // Returns handle to FIBITMAP either original or modified
    FIBITMAP* process(const ImageFrame& frame);

private:
    QWeakPointer<Image> mSrcImage;
    UniqueBitmap mProcessBuffer;
    QPixmap mDstPixmap;

    bool mIsValid = false;
    bool mIsBuffered = false;

    Rotation mRotation = Rotation::eDegree0;

    EnumArray<bool, FlipType> mFlips = { false };

    FREE_IMAGE_TMO mToneMapping = FITMO_CLAMP;

    double mGammaValue = 1.0;

    ChannelSwizzle mSwizzleType = ChannelSwizzle::eRGB;
};

#endif // IMAGEPROCESSOR_H
