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

#include "ImageProcessor.h"
#include <stdexcept>

namespace
{
    QImage makeQImageView(FIBITMAP* bmp)
    {
        assert(bmp != nullptr);
        QImage imageView;
        switch (FreeImage_GetBPP(bmp)) {
        case 1:
            imageView = QImage(FreeImage_GetBits(bmp), FreeImage_GetWidth(bmp), FreeImage_GetHeight(bmp), FreeImage_GetPitch(bmp), QImage::Format_Mono);
            break;
        case 8:
            imageView = QImage(FreeImage_GetBits(bmp), FreeImage_GetWidth(bmp), FreeImage_GetHeight(bmp), FreeImage_GetPitch(bmp), QImage::Format_Grayscale8);
            break;
        case 24:
            imageView = QImage(FreeImage_GetBits(bmp), FreeImage_GetWidth(bmp), FreeImage_GetHeight(bmp), FreeImage_GetPitch(bmp), QImage::Format_RGB888);
            break;
        case 32:
            imageView = QImage(FreeImage_GetBits(bmp), FreeImage_GetWidth(bmp), FreeImage_GetHeight(bmp), FreeImage_GetPitch(bmp), QImage::Format_RGBA8888);
            break;
        default:
            throw std::logic_error("Internal image must be 1, 8, 24 or 32 bit");
        }
        return imageView;
    }
}

ImageProcessor::ImageProcessor()
    : mProcessBuffer(nullptr, &::FreeImage_Unload)
{ }

ImageProcessor::~ImageProcessor() = default;

FIBITMAP* ImageProcessor::process(const ImageFrame& frame)
{
    FIBITMAP* target = frame.bmp;

    // 1. Tonemap
    auto imgType = FreeImage_GetImageType(target);
    if (imgType == FIT_RGBF || imgType == FIT_RGBAF || imgType == FIT_FLOAT || imgType == FIT_DOUBLE) {
        UniqueBitmap tonemapped(FreeImage_ToneMapping(target, mToneMapping), &::FreeImage_Unload);
        if (tonemapped) {
            mProcessBuffer = std::move(tonemapped);
            target = mProcessBuffer.get();
        }
    }

    // 2. Rotate
    if (mRotation != Rotation::eDegree0) {
        UniqueBitmap rotated(FreeImage_Rotate(target, static_cast<double>(toDegree(mRotation))), &::FreeImage_Unload);
        if (rotated) {
            mProcessBuffer = std::move(rotated);
            target = mProcessBuffer.get();
        }
    }

    // 3. Flip
    if (mFlips[FlipType::eHorizontal]) {
        if (target == frame.bmp) {
            mProcessBuffer.reset(FreeImage_Clone(frame.bmp));
            target = mProcessBuffer.get();
        }
        FreeImage_FlipHorizontal(target);
    }
    if (mFlips[FlipType::eVertical]) {
        if (target == frame.bmp) {
            mProcessBuffer.reset(FreeImage_Clone(frame.bmp));
            target = mProcessBuffer.get();
        }
        FreeImage_FlipVertical(target);
    }

    // 4. Gamma
    if (mGammaValue != 1.0) {
        imgType = FreeImage_GetImageType(target);
        if (imgType == FIT_BITMAP) {
            if (target == frame.bmp) {
                mProcessBuffer.reset(FreeImage_Clone(frame.bmp));
                target = mProcessBuffer.get();
            }
            FreeImage_AdjustGamma(target, 1.0 / mGammaValue);
        }
    }

    // 5. Swizzle
    if (mSwizzleType != ChannelSwizzle::eRGB) {
        UniqueBitmap swizzled(nullptr, &::FreeImage_Unload);
        switch(mSwizzleType) {
        case ChannelSwizzle::eBGR:
            if (target == frame.bmp) {
                mProcessBuffer.reset(FreeImage_Clone(frame.bmp));
                target = mProcessBuffer.get();
            }
            SwapRedBlue32(target);
            break;

        case ChannelSwizzle::eRed:
            swizzled.reset(FreeImage_GetChannel(target, FICC_RED));
            break;

        case ChannelSwizzle::eBlue:
            swizzled.reset(FreeImage_GetChannel(target, FICC_BLUE));
            break;

        case ChannelSwizzle::eGreen:
            swizzled.reset(FreeImage_GetChannel(target, FICC_GREEN));
            break;

        case ChannelSwizzle::eAlpha:
            swizzled.reset(FreeImage_GetChannel(target, FICC_ALPHA));
            break;

        default:
            break;
        }
        if (swizzled) {
            mProcessBuffer = std::move(swizzled);
            target = mProcessBuffer.get();
        }
    }

    mIsBuffered = (target == mProcessBuffer.get());

    return target;
}

const QPixmap& ImageProcessor::getResultPixmap()
{
    if (!mIsValid) {
        const auto pImg = mSrcImage.lock();
        if (pImg) {
            const ImageFrame& frame = pImg->getFrame();
            if (frame.bmp != nullptr) {
                mDstPixmap = QPixmap::fromImage(makeQImageView(process(frame)));
                mIsValid = true;
            }
        }
    }
    return mDstPixmap;
}

const UniqueBitmap& ImageProcessor::getResultBitmap()
{
    if (mIsValid && mIsBuffered) {
        return mProcessBuffer;
    }
    else {
        const auto pImg = mSrcImage.lock();
        if (pImg) {
            const ImageFrame& frame = pImg->getFrame();
            if (frame.bmp) {
                const auto bmp = process(frame);
                if (!mIsBuffered) {
                    mProcessBuffer.reset(FreeImage_Clone(bmp));
                    mIsBuffered = true;
                }
                mIsValid = true;
            }
        }
    }
    return mProcessBuffer;
}

void ImageProcessor::attachSource(QWeakPointer<Image> image)
{
    detachSource();
    mSrcImage = std::move(image);
    if (mSrcImage) {
        const auto pImg = mSrcImage.lock();
        if(pImg) {
            pImg->addListener(this);
        }
    }
}

void ImageProcessor::detachSource()
{
    if (mSrcImage) {
        const auto pImg = mSrcImage.lock();
        if(pImg) {
            pImg->removeListener(this);
        }
    }
    mProcessBuffer.reset();
    mIsValid = false;
}

void ImageProcessor::onInvalidated(Image* emitter)
{
    assert(emitter == mSrcImage.lock().get());
    (void)emitter;
    mIsValid = false;
}

uint32_t ImageProcessor::width() const
{
    return !mDstPixmap.isNull() ? mDstPixmap.width() : 0;
}

uint32_t ImageProcessor::height() const
{
    return !mDstPixmap.isNull() ? mDstPixmap.height() : 0;
}

bool ImageProcessor::getPixel(uint32_t y, uint32_t x, Pixel* p) const
{
    const auto pImg = mSrcImage.lock();
    bool success = false;
    if (pImg && p && y < height() && x < width()) {
        if (mFlips[FlipType::eHorizontal]) {
            x = width() - 1 - x;
        }
        if (mFlips[FlipType::eVertical]) {
            y = height() - 1 - y;
        }
        uint32_t srcY = y;
        uint32_t srcX = x;
        switch(mRotation) {
            case Rotation::eDegree90:
                srcY = x;
                srcX = pImg->width() - 1 - y;
                break;
            case Rotation::eDegree180:
                srcY = pImg->height() - 1 - y;
                srcX = pImg->width()  - 1 - x;
                break;
            case Rotation::eDegree270:
                srcY = pImg->height() - 1 - x;
                srcX = y;
                break;
            default:
                break;
        }
        if (srcX < pImg->width() && srcY < pImg->height()) {
            success = pImg->getPixel(pImg->height() - 1 - srcY, srcX, p);
            if (success) {
                p->y = srcY;
                p->x = srcX;
            }
        }
    }
    return success;
}

