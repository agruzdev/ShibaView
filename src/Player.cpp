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

#include "Player.h"

#include <cassert>
#include <stdexcept>
#include <vector>

#include "ImageSource.h"
#include "FreeImageExt.h"

namespace
{
    constexpr size_t kMaxCacheBytes = 128 * 1024 * 1024;
}

struct Player::ConvertionContext
{
    ImageSource::ImagePagePtr page;
    ImageFrame frame{};
    bool needsUnload = false;

    ConvertionContext(ImageSource::ImagePagePtr page)
        : page(std::move(page))
    { }

    ConvertionContext(const ConvertionContext&) = delete;

    ConvertionContext(ConvertionContext&&) = delete;

    ~ConvertionContext()
    {
        if (needsUnload && frame.bmp) {
            FreeImage_Unload(frame.bmp);
        }
    }

    ConvertionContext& operator=(const ConvertionContext&) = delete;

    ConvertionContext& operator=(ConvertionContext&&) = delete;
};



Player::Player(std::shared_ptr<ImageSource> src)
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

std::unique_ptr<Player::ConvertionContext> Player::loadZeroFrame(ImageSource* source)
{
    const uint32_t pagesNum = source->pagesCount();
    assert(pagesNum > 0);

    auto finfo = std::make_unique<ConvertionContext>(source->lockPage(0));
    if (!finfo->page) {
        throw std::runtime_error("Player[loadZeroFrame]: Failed to read image source.");
    }

    finfo->frame = cvtToInternalType(finfo->page->getBitmap(), finfo->needsUnload);
    if (!finfo->frame.bmp) {
        throw std::runtime_error("Player[loadZeroFrame]: Failed to convert a frame.");
    }

    finfo->frame.page = finfo->page.get();
    finfo->frame.index = 0;
    if (pagesNum > 1) {
        finfo->frame.duration = finfo->page->getAnimation().duration;
    }

    return finfo;
}

std::unique_ptr<Player::ConvertionContext> Player::loadNextFrame(ImageSource* source, const ConvertionContext* prev)
{
    assert(prev != nullptr);
    const uint32_t prevIdx = prev->frame.index;
    const uint32_t nextIdx = (prevIdx + 1) % mSource->pagesCount();

    auto next = std::make_unique<ConvertionContext>(source->lockPage(nextIdx));
    if (!next->page) {
        throw std::runtime_error("Player[loadNextFrame]: Failed to decode the next page.");
    }

    next->frame = cvtToInternalType(next->page->getBitmap(), next->needsUnload);
    if (!next->frame.bmp) {
        throw std::runtime_error("Player[loadNextFrame]: Failed to convert the next frame.");
    }
 
    if (source->storesDiffernece()) {
        FIBITMAP* canvas = FreeImage_Clone(prev->frame.bmp);
        const auto& nextAnim = next->page->getAnimation();
        const auto drawSuccess = FreeImageExt_Draw(canvas, next->frame.bmp, FIEAF_SrcAlpha, nextAnim.offsetX, nextAnim.offsetY);

        if (next->needsUnload) {
            FreeImage_Unload(next->frame.bmp);
        }
        next->frame.bmp = canvas;
        next->needsUnload = true;

        if (!drawSuccess) {
            throw std::runtime_error("Player[loadNextFrame]: Failed to combine frames.");
        }
    }

    next->frame.page     = next->page.get();
    next->frame.index    = nextIdx;
    next->frame.duration = next->page->getAnimation().duration;

    return next;
}

bool Player::getPixel(uint32_t y, uint32_t x, Pixel* p) const
{
    if (mCacheIndex < mFramesCache.size()) {
        auto& frame = mFramesCache[mCacheIndex];
        if (mSource->storesDiffernece()) {
            // Impossible to fetch original color without blending
            if (frame->frame.bmp) {
                return Pixel::getBitmapPixel(frame->frame.bmp, y, x, p);
            }
        }
        else {
            // Use original page
            if (frame->page) {
                return frame->page->getPixel(y, x, p);
            }
        }
    }
    return false;
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
            std::unique_ptr<ConvertionContext> buffer = nullptr;
            ConvertionContext* frame = nullptr;
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

            std::vector<std::unique_ptr<ConvertionContext>> newFrames;
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
        frame.flags = FrameFlags::eHRD | FrameFlags::eRGB;
        frame.bmp = src;
        dstNeedUnload = false;
        break;

    case FIT_RGBF:
        frame.flags = FrameFlags::eHRD | FrameFlags::eRGB;
        frame.bmp = src;
        dstNeedUnload = false;
        break;

    case FIT_RGBA16:
    case FIT_RGBA32:
        frame.flags = FrameFlags::eHRD | FrameFlags::eRGB;
        frame.bmp = FreeImage_ConvertToRGBAF(src);
        dstNeedUnload = true;
        break;

    case FIT_RGB16:
    case FIT_RGB32:
        frame.flags = FrameFlags::eHRD | FrameFlags::eRGB;
        frame.bmp = FreeImage_ConvertToRGBF(src);
        dstNeedUnload = true;
        break;

    case FIT_UINT16:
    case FIT_INT16:
    case FIT_UINT32:
    case FIT_INT32:
        frame.bmp = FreeImageExt_ConvertToFloat(src);
        frame.flags = FrameFlags::eHRD;
        dstNeedUnload = true;
        break;

    case FIT_FLOAT:
        frame.flags = FrameFlags::eHRD;
        frame.bmp = src;
        dstNeedUnload = false;
        break;

    case FIT_DOUBLE:
        frame.flags = FrameFlags::eHRD;
        frame.bmp = src;
        dstNeedUnload = false;
        break;

    case FIT_BITMAP:
        if (32 == bpp) {
            frame.flags = FrameFlags::eRGB;
            frame.bmp = src;
            dstNeedUnload = false;
        }
        else if (24 == bpp) {
            frame.flags = FrameFlags::eRGB;
            frame.bmp = src;
            dstNeedUnload = false;
        }
        else if (8 == bpp) {
            const auto colorType = FreeImage_GetColorType(src);
            if (FIC_PALETTE == colorType) {
                //FreeImage_Save(FIF_TIFF, src, "test.tiff");

                frame.flags = FrameFlags::eRGB;
                frame.bmp = FreeImage_ConvertTo32Bits(src);
                dstNeedUnload = true;
            }
            else if (FIC_MINISWHITE == colorType) {
                frame.bmp = FreeImage_Clone(src);
                FreeImage_Invert(frame.bmp);
                dstNeedUnload = true;
            }
            else {
                frame.bmp = src;
                dstNeedUnload = false;
            }
        }
        else if(4 == bpp) {
            frame.bmp = FreeImage_ConvertTo32Bits(src);
            frame.flags = FrameFlags::eRGB;
            dstNeedUnload = true;
        }
        else if(1 == bpp) {
            const auto colorType = FreeImage_GetColorType(src);
            if (FIC_PALETTE == colorType) {
                frame.flags = FrameFlags::eRGB;
                frame.bmp = FreeImage_ConvertTo32Bits(src);
                dstNeedUnload = true;
            }
            else if (FIC_MINISWHITE == colorType) {
                frame.bmp = FreeImage_Clone(src);
                FreeImage_Invert(frame.bmp);
                dstNeedUnload = true;
            }
            else {
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


UniqueBitmap Player::getOrMakeThumbnail(FIBITMAP* src, uint32_t maxSize)
{
    UniqueBitmap result(nullptr, &FreeImage_Unload);
    if (src) {
        if (FIBITMAP* storedThumbnail = FreeImage_GetThumbnail(src)) {
            const unsigned w = FreeImage_GetWidth(storedThumbnail);
            const unsigned h = FreeImage_GetHeight(storedThumbnail);
            if (w > maxSize || h > maxSize) {
                const unsigned size = std::max(w, h);
                result.reset(FreeImage_Rescale(storedThumbnail, w * maxSize / size, h * maxSize / size, FILTER_BICUBIC));
            }
        }
        else {
            bool needToUnload = false;
            auto internalFrame = cvtToInternalType(src, needToUnload);
            if (internalFrame.bmp) {
                FIBITMAP* ldrFrame = internalFrame.bmp;
                if ((internalFrame.flags & FrameFlags::eHRD) != FrameFlags::eNone) {
                    ldrFrame = FreeImageExt_ToneMapping(internalFrame.bmp, FIETMO_LINEAR);
                }
                if (ldrFrame) {
                    const unsigned w = FreeImage_GetWidth(ldrFrame);
                    const unsigned h = FreeImage_GetHeight(ldrFrame);
                    const unsigned size = std::max(w, h);
                    result.reset(FreeImage_Rescale(ldrFrame, w * maxSize / size, h * maxSize / size, FILTER_BICUBIC));
                }
                if (ldrFrame != internalFrame.bmp) {
                    FreeImage_Unload(ldrFrame);
                }
                if (needToUnload) {
                    FreeImage_Unload(internalFrame.bmp);
                }
            }
        }
    }
    return result;
}
