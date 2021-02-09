/**
 * @file
 *
 * Copyright 2018-2021 Alexey Gruzdev
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
#include "Utilities.h"
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
            throw std::logic_error("Internal image is 1, 8, 24 or 32 bit");
        }
        return imageView;
    }
}

ImageProcessor::ImageProcessor() = default;

ImageProcessor::~ImageProcessor() = default;

const QPixmap & ImageProcessor::getResult()
{
    const auto pImg = mSrcImage.lock();
    if (pImg) {
        const ImageFrame & frame = pImg->getFrame();
        if (!mDstPixmap || !mIsValid) {
            FIBITMAP* target = frame.bmp;

            std::unique_ptr<FIBITMAP, decltype(&::FreeImage_Unload)> rotated(nullptr, &::FreeImage_Unload);
            if (mRotation != Rotation::eDegree0) {
                rotated.reset(FreeImage_Rotate(frame.bmp, static_cast<double>(toDegree(mRotation))));
                if (rotated) {
                    target = rotated.get();
                }
            }

            std::unique_ptr<FIBITMAP, decltype(&::FreeImage_Unload)> tonemapped(nullptr, &::FreeImage_Unload);
            const auto imgType = FreeImage_GetImageType(target);
            if (imgType == FIT_RGBF || imgType == FIT_RGBAF || imgType == FIT_FLOAT || imgType == FIT_DOUBLE) {
                tonemapped.reset(FreeImageExt_ToneMapping(target, mToneMapping));
                if (tonemapped) {
                    target = tonemapped.get();
                }
            }

            std::unique_ptr<FIBITMAP, decltype(&::FreeImage_Unload)> swizzled(nullptr, &::FreeImage_Unload);
            if (mSwizzleType != ChannelSwizzle::eRGB) {
                switch(mSwizzleType) {
                case ChannelSwizzle::eBGR:
                    swizzled.reset(FreeImage_Clone(target));
                    SwapRedBlue32(swizzled.get());
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
                    target = swizzled.get();
                }
            }

            mDstPixmap = QPixmap::fromImage(makeQImageView(target));
            mIsValid = true;
        }
    }
    return mDstPixmap;
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

