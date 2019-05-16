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
#include <vector>

#include "ImageSource.h"
#include "FreeImageExt.h"

namespace
{
    constexpr size_t kMaxCacheBytes = 128 * 1024 * 1024;
}

struct Player::FrameInfo
{
    std::shared_ptr<FIBITMAP> page = nullptr;
    ImageFrame frame{};
    bool needsUnload = false;
};

void Player::FrameInfoDeleter::operator()(Player::FrameInfo* p) const
{
    if(p) {
        if (p->needsUnload && p->frame.bmp) {
            FreeImage_Unload(p->frame.bmp);
        }
        delete p;
    }
}

Player::FrameInfoPtr Player::newFrameInfo()
{
    return FrameInfoPtr(new FrameInfo, FrameInfoDeleter{});
}


Player::Player(std::unique_ptr<ImageSource> && src)
    //: mSource(std::move(src))
{
    if (!src) {
        throw std::runtime_error("Player[Player]: Image source is null.");
    }

    const auto framesNum = src->pagesCount();
    if (framesNum > 0) {
        try {
            mFramesCache.push_back(loadZeroFrame(src.get()));
            mCacheIndex = 0;
        }
        catch(...) {
            mFramesCache.clear();
            throw;
        }

        const size_t frameSize = FreeImage_GetMemorySize(mFramesCache[0]->frame.bmp);
        mMaxCacheSize = std::max(static_cast<size_t>(1), kMaxCacheBytes / frameSize);
    }

    mSource = std::move(src);
}

Player::~Player()
{
    mFramesCache.clear();
    mSource.reset();
}

Player::FrameInfoPtr Player::loadZeroFrame(ImageSource* source)
{
    const uint32_t pagesNum = source->pagesCount();
    assert(pagesNum > 0);

    auto finfo = newFrameInfo();

    AnimationInfo anim{};
    finfo->page = source->lockPage(0, &anim);
    if (!finfo->page) {
        throw std::runtime_error("Player[Player]: Failed to read image source.");
    }

    finfo->frame = cvtToInternalType(finfo->page.get(), finfo->needsUnload);
    if (!finfo->frame.bmp) {
        throw std::runtime_error("Player[Player]: Failed to convert a frame.");
    }

    finfo->frame.index = 0;
    if (pagesNum > 1) {
        finfo->frame.duration = anim.duration;
    }

    // Release page early if bitmap owns data
    if (finfo->needsUnload) {
        finfo->page.reset();
    }

    return finfo;
}

Player::FrameInfoPtr Player::loadNextFrame(ImageSource* source, const FrameInfo* prev)
{
    auto next = newFrameInfo();

    const uint32_t prevIdx = prev->frame.index;
    const uint32_t nextIdx = (prevIdx + 1) % mSource->pagesCount();

    AnimationInfo nextAnim{};
    next->page = source->lockPage(nextIdx, &nextAnim);
    if (!next->page) {
        throw std::runtime_error("Player[next]: Failed to decode the next page.");
    }

    next->frame = cvtToInternalType(next->page.get(), next->needsUnload);
    if (!next->frame.bmp) {
        throw std::runtime_error("Player[next]: Failed to convert the next frame.");
    }

    if (next->needsUnload) {
        next->page = nullptr;
    }

    if (source->storesDiffernece()) {
        FIBITMAP* canvas = FreeImage_Clone(prev->frame.bmp);
        const auto drawSuccess = FreeImageExt_Draw(canvas, next->frame.bmp, FIEAF_SrcAlpha, nextAnim.offsetX, nextAnim.offsetY);

        if (next->needsUnload) {
            FreeImage_Unload(next->frame.bmp);
        }
        next->frame.bmp = canvas;
        next->needsUnload = true;

        if (!drawSuccess) {
            throw std::runtime_error("Player[Player]: Failed to combine frames.");
        }
    }

    next->frame.index    = nextIdx;
    next->frame.duration = nextAnim.duration;

    return next;
}


ImageFrame* Player::getImpl() const
{
    if (mCacheIndex < mFramesCache.size()) {
        return &(mFramesCache[mCacheIndex]->frame);
    }
    else {
        return nullptr;
    }
}

const ImageFrame & Player::getCurrentFrame() const
{
    const auto* frame = getImpl();
    if (!frame) {
        throw std::runtime_error("Player[getCurrentFrame]: No frames available.");
    }
    return *frame;
}

void Player::next()
{
    if (mSource->pagesCount() > 1) {

        const uint32_t prevIdx = getCurrentFrame().index;
        const uint32_t nextIdx = (prevIdx + 1) % mSource->pagesCount();

        if (mCacheIndex < mFramesCache.size() - 1) {
            // Already cached
            ++mCacheIndex;
        }
        else if(mFramesCache.front()->frame.index == nextIdx) {
            // Found in head
            mCacheIndex = 0;
        }
        else  {
            // Load and save in tail
            auto next = loadNextFrame(mSource.get(), mFramesCache.at(mCacheIndex).get());
            if (next) {
                mFramesCache.push_back(std::move(next));
                if (mFramesCache.size() > mMaxCacheSize) {
                    mFramesCache.pop_front();
                }
                mCacheIndex = mFramesCache.size() - 1;
            }
        }
    }
}

void Player::prev()
{
    if (mSource->pagesCount() > 1) {

        const uint32_t prevIdx = getCurrentFrame().index;
        const uint32_t nextIdx = prevIdx == 0 ? mSource->pagesCount() - 1 : prevIdx - 1;

        if (mCacheIndex > 0) {
            // Already cached
            --mCacheIndex;
        }
        else if(mFramesCache.back()->frame.index == nextIdx) {
            // Found in head
            mCacheIndex = mFramesCache.size() - 1;
        }
        else  {
            // Find closest previous frame
            FrameInfoPtr buffer = nullptr;
            FrameInfo* frame = nullptr;
            if (nextIdx > mFramesCache.back()->frame.index) {
                buffer = loadNextFrame(mSource.get(), mFramesCache.back().get());
            }
            else {
                buffer = loadZeroFrame(mSource.get());
            }
            frame = buffer.get();

            // Load
            const uint32_t countToCache = static_cast<uint32_t>(std::max(2 * mMaxCacheSize / 3, mMaxCacheSize - mFramesCache.size()));
            const uint32_t cacheFromIdx = countToCache < nextIdx ? nextIdx - countToCache : 0; // add to cache frames with index >= cacheFromIdx

            size_t cachedCount = mFramesCache.size();

            std::vector<FrameInfoPtr> newFrames;
            if (buffer->frame.index >= cacheFromIdx) {
                newFrames.push_back(std::move(buffer));
                ++cachedCount;
                if (cachedCount > mMaxCacheSize) {
                    mFramesCache.pop_back();
                }
            }

            while (frame->frame.index < nextIdx) {
                buffer = loadNextFrame(mSource.get(), frame);
                frame = buffer.get();
                if (buffer->frame.index >= cacheFromIdx) {
                    newFrames.push_back(std::move(buffer));
                    ++cachedCount;
                    if (cachedCount > mMaxCacheSize) {
                        mFramesCache.pop_back();
                    }
                }
            }

            if (newFrames.empty() || newFrames.back()->frame.index != nextIdx) {
                throw std::logic_error("Player[prev]: Cache was corrupted. Flushing it.");
            }

            mCacheIndex = newFrames.size() - 1;
            mFramesCache.insert(mFramesCache.cbegin(), std::make_move_iterator(newFrames.begin()), std::make_move_iterator(newFrames.end()));
        }
    }
}


uint32_t Player::framesNumber() const
{
    return mSource->pagesCount();
}

uint32_t Player::getWidth() const
{
    const auto* frame = getImpl();
    if (frame) {
        return static_cast<uint32_t>(FreeImage_GetWidth(frame->bmp));
    }
    else {
        return 0;
    }
}

uint32_t Player::getHeight() const
{
    const auto* frame = getImpl();
    if (frame) {
        return static_cast<uint32_t>(FreeImage_GetHeight(frame->bmp));
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
