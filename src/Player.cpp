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

Player::Player(std::unique_ptr<ImageSource> && src)
    : mSource(std::move(src))
{
    if (!mSource) {
        throw std::runtime_error("Player[Player]: Image source is null.");
    }

    const auto framesNum = framesNumber();
    if (framesNum > 0) {
        mCurrentPage = mSource->decodePage(0, &mCurrentAnimation);
        if(!mCurrentPage) {
            throw std::runtime_error("Player[Player]: Failed to read image source.");
        }
        try {
            mCurrentFrame = cvtToInternalType(mCurrentPage, mFrameNeedsUnload);
            if (!mCurrentFrame.bmp) {
                throw std::runtime_error("Player[Player]: Failed to convert a frame.");
            }

            mCurrentFrame.index = 0;
            if (framesNum > 1) {
                mCurrentFrame.duration = mCurrentAnimation.duration;
            }

            // Release page early if bitmap owns data
            if (mFrameNeedsUnload) {
                mSource->releasePage(mCurrentPage);
                mCurrentPage = nullptr;
            }
        }
        catch(...) {
            mSource->releasePage(mCurrentPage);
            throw;
        }
    }

}

Player::~Player()
{
    if (mFrameNeedsUnload) {
        FreeImage_Unload(mCurrentFrame.bmp);
    }
    if (mSource && mCurrentPage) {
        mSource->releasePage(mCurrentPage);
    }
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

        FIBITMAP* prevBmp = nullptr;
        if (mCurrentFrame.bmp && mSource->storesResidual()) {
            prevBmp = FreeImage_ConvertTo24Bits(mCurrentFrame.bmp);
        }

        // Free old internal bitmap, if it owns data
        if (mFrameNeedsUnload) {
            FreeImage_Unload(mCurrentFrame.bmp);
            mFrameNeedsUnload = false;
        }
        mCurrentFrame.bmp = nullptr;
        // Release old page if it is held
        if (mCurrentPage) {
            mSource->releasePage(mCurrentPage);
            mCurrentPage = nullptr;
        }
        // decode current page
        mCurrentPage = mSource->decodePage(nextIdx, &mCurrentAnimation);
        if (!mCurrentPage) {
            throw std::runtime_error("Player[next]: Failed to decode the next page.");
        }

        // Convert to internal bitmap
        mCurrentFrame = cvtToInternalType(mCurrentPage, mFrameNeedsUnload);
        if (!mCurrentFrame.bmp) {
            throw std::runtime_error("Player[Player]: Failed to convert a frame.");
        }

        if (prevBmp) {
            const int32_t dw = FreeImage_GetWidth(prevBmp)  - FreeImage_GetWidth(mCurrentFrame.bmp);
            const int32_t dh = FreeImage_GetHeight(prevBmp) - FreeImage_GetHeight(mCurrentFrame.bmp);
            if (dw != 0 || dh != 0) {
                // Apply foreground offset
                RGBQUAD transparent {};
                transparent.rgbReserved = 0x00;
                // Internal image is vertically flipped -> offsetY is applied from below
                //FIBITMAP* foreground = FreeImage_EnlargeCanvas(mCurrentFrame.bmp, mCurrentAnimation.offsetX, dh - mCurrentAnimation.offsetY, dw - mCurrentAnimation.offsetX, mCurrentAnimation.offsetY, &transparent, FI_COLOR_IS_RGBA_COLOR);
                FIBITMAP* foreground = FreeImage_EnlargeCanvas(mCurrentFrame.bmp, mCurrentAnimation.offsetX, mCurrentAnimation.offsetY, dw - mCurrentAnimation.offsetX, dh - mCurrentAnimation.offsetY, &transparent, FI_COLOR_IS_RGBA_COLOR);
                if (foreground) {
                    if (mFrameNeedsUnload) {
                        FreeImage_Unload(mCurrentFrame.bmp);
                    }
                    mCurrentFrame.bmp = foreground;
                    mFrameNeedsUnload = true;
                }
            }
            FIBITMAP* composed = FreeImage_Composite(mCurrentFrame.bmp, FALSE, nullptr, prevBmp);
            FreeImage_Unload(prevBmp);
            if (composed) {
                if (mFrameNeedsUnload) {
                    FreeImage_Unload(mCurrentFrame.bmp);
                }
                mCurrentFrame.bmp = composed;
                mFrameNeedsUnload = true;
            }
        }

        // Release page early if bitmap owns data
        if (mFrameNeedsUnload) {
            mSource->releasePage(mCurrentPage);
            mCurrentPage = nullptr;
        }

        mCurrentFrame.index    = nextIdx;
        mCurrentFrame.duration = mCurrentAnimation.duration;
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
