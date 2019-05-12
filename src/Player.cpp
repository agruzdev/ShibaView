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

#include "Player.h"

#include <cassert>
#include "ImageSource.h"
#include "FreeImageExt.h"


Player::Player(std::unique_ptr<ImageSource> && src)
    //: mSource(std::move(src))
{
    if (!src) {
        throw std::runtime_error("Player[Player]: Image source is null.");
    }

    const auto framesNum = src->pagesCount();
    if (framesNum > 0) {
        AnimationInfo anim{};
        mCurrentPage = src->lockPage(0, &anim);
        if (!mCurrentPage) {
            throw std::runtime_error("Player[Player]: Failed to read image source.");
        }
        try {
            mCurrentFrame = cvtToInternalType(mCurrentPage.get(), mFrameNeedsUnload);
            if (!mCurrentFrame.bmp) {
                throw std::runtime_error("Player[Player]: Failed to convert a frame.");
            }

            mCurrentFrame.index = 0;
            if (framesNum > 1) {
                mCurrentFrame.duration = anim.duration;
            }

            // Release page early if bitmap owns data
            if (mFrameNeedsUnload) {
                mCurrentPage.reset();
            }
        }
        catch(...) {
            mCurrentPage = nullptr;
            throw;
        }
    }

    mSource = std::move(src);
}

Player::~Player()
{
    if (mFrameNeedsUnload) {
        FreeImage_Unload(mCurrentFrame.bmp);
    }
    mCurrentPage = nullptr;
}

const ImageFrame & Player::getCurrentFrame() const
{
    return mCurrentFrame;
}

void Player::next()
{
    if (mSource->pagesCount() > 1) {

        const uint32_t prevIdx = mCurrentFrame.index;
        const uint32_t nextIdx = (prevIdx + 1) % mSource->pagesCount();

        AnimationInfo nextAnim{};
        auto nextPage = mSource->lockPage(nextIdx, &nextAnim);
        if (!nextPage) {
            throw std::runtime_error("Player[next]: Failed to decode the next page.");
        }

        bool nextNeedsUnload = false;
        ImageFrame nextFrame = cvtToInternalType(nextPage.get(), nextNeedsUnload);
        if (!nextFrame.bmp) {
            throw std::runtime_error("Player[next]: Failed to convert the next frame.");
        }

        if (nextNeedsUnload) {
            nextPage = nullptr;
        }

        if (mSource->storesDiffernece()) {
            if (!mFrameNeedsUnload) {
                mCurrentFrame.bmp = FreeImage_Clone(mCurrentFrame.bmp);
                mFrameNeedsUnload = true;
            }

            const auto drawSuccess = FreeImageExt_Draw(mCurrentFrame.bmp, nextFrame.bmp, FIEAF_SrcAlpha, nextAnim.offsetX, nextAnim.offsetY);

            if (nextNeedsUnload) {
                FreeImage_Unload(nextFrame.bmp);
            }

            if(!drawSuccess) {
                throw std::runtime_error("Player[Player]: Failed to combine frames.");
            }

            nextFrame.bmp = mCurrentFrame.bmp;
            nextNeedsUnload = true;
            mFrameNeedsUnload = false;
        }

        if (mFrameNeedsUnload) {
            FreeImage_Unload(mCurrentFrame.bmp);
        }

        mCurrentPage      = nextPage;
        mCurrentFrame     = nextFrame;
        mFrameNeedsUnload = nextNeedsUnload;

        mCurrentFrame.index    = nextIdx;
        mCurrentFrame.duration = nextAnim.duration;
    }
}

uint32_t Player::framesNumber() const
{
    return mSource->pagesCount();
}

uint32_t Player::getWidth() const
{
    if (mCurrentFrame.bmp) {
        return static_cast<uint32_t>(FreeImage_GetWidth(mCurrentFrame.bmp));
    }
    else {
        return 0;
    }
}

uint32_t Player::getHeight() const
{
    if (mCurrentFrame.bmp) {
        return static_cast<uint32_t>(FreeImage_GetHeight(mCurrentFrame.bmp));
    }
    else {
        return 0;
    }
}

ImageFrame Player::cvtToInternalType(FIBITMAP* src, bool & dstNeedUnload)
{
    assert(src != nullptr);
    ImageFrame frame{};
    const uint32_t bpp = FreeImage_GetBPP(src);
    switch (FreeImage_GetImageType(src)) {
    case FIT_RGBAF:
        frame.srcFormat = "RGBA float";
        frame.flags = FrameFlags::eHRD | FrameFlags::eRGB;
        frame.bmp = src;
        dstNeedUnload = false;
        break;

    case FIT_RGBF:
        frame.srcFormat = "RGB float";
        frame.flags = FrameFlags::eHRD | FrameFlags::eRGB;
        frame.bmp = src;
        dstNeedUnload = false;
        break;

    case FIT_RGBA16:
        assert(bpp == 64);
        frame.srcFormat = "RGBA16";
        frame.flags = FrameFlags::eRGB;
        frame.bmp = FreeImage_ConvertTo32Bits(src);
        dstNeedUnload = true;
        break;

    case FIT_RGB16:
        assert(bpp == 48);
        frame.srcFormat = "RGB16";
        frame.flags = FrameFlags::eRGB;
        frame.bmp = FreeImage_ConvertTo24Bits(src);
        dstNeedUnload = true;
        break;

    case FIT_UINT16:
        frame.srcFormat = "Greyscale 16bit";
        goto ConvertToStandardType;
    case FIT_INT16:
        frame.srcFormat = "Greyscale 16bit (signed)";
        goto ConvertToStandardType;
    case FIT_UINT32:
        frame.srcFormat = "Greyscale 32bit";
        goto ConvertToStandardType;
    case FIT_INT32:
        frame.srcFormat = "Greyscale 32bit (signed)";
        goto ConvertToStandardType;

    ConvertToStandardType:
        frame.bmp = FreeImage_ConvertToStandardType(src);
        dstNeedUnload = true;
        break;


    case FIT_FLOAT:
        frame.srcFormat = "Greyscale float";
        frame.flags = FrameFlags::eHRD;
        frame.bmp = src;
        dstNeedUnload = false;
        break;

    case FIT_DOUBLE:
        frame.srcFormat = "Greyscale double";
        frame.flags = FrameFlags::eHRD;
        frame.bmp = src;
        dstNeedUnload = false;
        break;

    case FIT_BITMAP:
        if (32 == bpp) {
            frame.srcFormat = "RGBA8888";
            frame.flags = FrameFlags::eRGB;
            frame.bmp = src;
            dstNeedUnload = false;
        }
        else if (24 == bpp) {
            frame.srcFormat = "RGB888";
            frame.flags = FrameFlags::eRGB;
            frame.bmp = src;
            dstNeedUnload = false;
        }
        else if (8 == bpp) {
            if (FIC_PALETTE == FreeImage_GetColorType(src)) {
                frame.srcFormat = "RGB Indexed 8bit";
                frame.flags = FrameFlags::eRGB;
                frame.bmp = FreeImage_ConvertTo32Bits(src);
                dstNeedUnload = true;
            }
            else {
                frame.srcFormat = "Greyscale 8bit";
                frame.bmp = src;
                dstNeedUnload = false;
            }
        }
        else if(4 == bpp) {
            frame.srcFormat = "RGB Indexed 4bit";
            frame.bmp = FreeImage_ConvertTo32Bits(src);
            frame.flags = FrameFlags::eRGB;
            dstNeedUnload = true;
        }
        else if(1 == bpp) {
            if (FIC_PALETTE == FreeImage_GetColorType(src)) {
                frame.srcFormat = "RGB Indexed 1bit";
                frame.flags = FrameFlags::eRGB;
                frame.bmp = FreeImage_ConvertTo32Bits(src);
                dstNeedUnload = true;
            }
            else {
                frame.srcFormat = "Binary image";
                frame.bmp = src;
                dstNeedUnload = false;
            }
        }
        break;

    default:
        break;
    }
    return frame;
}
