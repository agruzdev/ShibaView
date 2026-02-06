/**
 * @file
 *
 * Copyright 2018-2026 Alexey Gruzdev
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
#include "Pixel.h"
#include "ImagePage.h"
#include "FreeImageExt.h"

namespace
{
    constexpr size_t kMaxCacheBytes = 256 * 1024 * 1024;
}


struct Player::CacheEntry
{
    ImageSource::ImagePagePtr page;
    UniqueBitmap blendedImage;

    CacheEntry(ImageSource::ImagePagePtr page)
        : page(std::move(page))
        , blendedImage(nullptr, &::FreeImage_Unload)
    { }
};



Player::Player(std::shared_ptr<ImageSource> src)
{
    if (!src) {
        throw std::runtime_error("Player[Player]: Image source is null.");
    }

    const auto framesNum = src->pagesCount();
    if (framesNum > 0) {
        try {
            mFramesCache.emplace_back(loadZeroFrame(src.get()));
            mCacheIndex = 0;
        }
        catch(...) {
            mFramesCache.clear();
            throw;
        }

        const size_t frameSize = mFramesCache[0]->page->getMemorySize();
        mMaxCacheSize = std::max(static_cast<size_t>(1), kMaxCacheBytes / frameSize);
    }

    mSource = std::move(src);
}

Player::~Player()
{
    mFramesCache.clear();
    mSource.reset();
}

std::unique_ptr<Player::CacheEntry> Player::loadZeroFrame(ImageSource* source)
{
    auto entry = std::make_unique<CacheEntry>(source->lockPage(0));
    if (!entry->page) {
        throw std::runtime_error("Player[loadZeroFrame]: Failed to decode zero page.");
    }
    return entry;
}

std::unique_ptr<Player::CacheEntry> Player::loadNextFrame(ImageSource* source, const CacheEntry& prev)
{
    assert(prev.page != nullptr);
    const uint32_t prevIdx = prev.page->index();
    const uint32_t nextIdx = (prevIdx + 1) % mSource->pagesCount();

    auto nextEntry = std::make_unique<CacheEntry>(source->lockPage(nextIdx));
    if (!nextEntry->page) {
        throw std::runtime_error("Player[loadNextFrame]: Failed to decode the next page.");
    }

    if (source->storesDifference()) {
        const auto& nextAnim = nextEntry->page->animation();
        if (nextAnim.disposal == DisposalType::eLeave) {
            UniqueBitmap canvas(FreeImage_Clone(prev.blendedImage ? prev.blendedImage.get() : prev.page->getBitmap()), &::FreeImage_Unload);
            const auto drawSuccess = FreeImage_DrawBitmap(canvas.get(), nextEntry->page->getBitmap(), FIAO_SrcAlpha, nextAnim.offsetX, nextAnim.offsetY);
            if (!drawSuccess) {
                throw std::runtime_error("Player[loadNextFrame]: Failed to combine frames.");
            }
            nextEntry->blendedImage = std::move(canvas);
        }
        else if (nextAnim.disposal == DisposalType::eBackground) {
            // do nothing, use frame as it is
        }
        else {
            // not implemented...
        }
    }

    return nextEntry;
}

const ImagePage& Player::getCurrentPage() const
{
    if (mCacheIndex < mFramesCache.size()) {
        return *(mFramesCache[mCacheIndex]->page);
    }
    throw std::runtime_error("Player[getCurrentFrame]: No pages are available.");
}

FIBITMAP* Player::getBlendedBitmap() const
{
    if (mCacheIndex < mFramesCache.size()) {
        return mFramesCache[mCacheIndex]->blendedImage ? mFramesCache[mCacheIndex]->blendedImage.get() : mFramesCache[mCacheIndex]->page->getBitmap();
    }
    throw std::runtime_error("Player[getCurrentFrame]: No pages are available.");
}

void Player::next()
{
    if (mSource->pagesCount() > 1) {

        const uint32_t prevIdx = getCurrentPage().index();
        const uint32_t nextIdx = (prevIdx + 1) % mSource->pagesCount();

        if (mCacheIndex < mFramesCache.size() - 1) {
            // Already cached
            ++mCacheIndex;
        }
        else if(mFramesCache.front()->page->index() == nextIdx) {
            // Found in head
            mCacheIndex = 0;
        }
        else  {
            // Load and save in tail
            auto next = loadNextFrame(mSource.get(), *mFramesCache.back());
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

        const uint32_t prevIdx = getCurrentPage().index();
        const uint32_t nextIdx = prevIdx == 0 ? mSource->pagesCount() - 1 : prevIdx - 1;

        if (mCacheIndex > 0) {
            // Already cached
            --mCacheIndex;
        }
        else if(mFramesCache.back()->page->index() == nextIdx) {
            // Found in head
            mCacheIndex = mFramesCache.size() - 1;
        }
        else  {
            // Find closest previous frame
            std::unique_ptr<CacheEntry> buffer = nullptr;
            CacheEntry* lastEntry = nullptr;
            if (nextIdx > mFramesCache.back()->page->index()) {
                buffer = loadNextFrame(mSource.get(), *mFramesCache.back());
            }
            else {
                buffer = loadZeroFrame(mSource.get());
            }
            lastEntry = buffer.get();

            // Load
            const uint32_t countToCache = static_cast<uint32_t>(std::max(2 * mMaxCacheSize / 3, mMaxCacheSize - mFramesCache.size()));
            const uint32_t cacheFromIdx = countToCache < nextIdx ? nextIdx - countToCache : 0; // add to cache frames with index >= cacheFromIdx

            size_t cachedCount = mFramesCache.size();

            std::vector<std::unique_ptr<CacheEntry>> newFrames;
            if (lastEntry->page->index() >= cacheFromIdx) {
                newFrames.push_back(std::move(buffer));
                ++cachedCount;
                if (cachedCount > mMaxCacheSize) {
                    mFramesCache.pop_back();
                }
            }

            while (lastEntry->page->index() < nextIdx) {
                buffer = loadNextFrame(mSource.get(), *lastEntry);
                lastEntry = buffer.get();
                if (lastEntry->page->index() >= cacheFromIdx) {
                    newFrames.push_back(std::move(buffer));
                    ++cachedCount;
                    if (cachedCount > mMaxCacheSize) {
                        mFramesCache.pop_back();
                    }
                }
            }

            if (newFrames.empty() || newFrames.back()->page->index() != nextIdx) {
                throw std::logic_error("Player[prev]: Cache was corrupted.");
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
    return static_cast<uint32_t>(FreeImage_GetWidth(getCurrentPage().getBitmap()));
}

uint32_t Player::getHeight() const
{
    return static_cast<uint32_t>(FreeImage_GetHeight(getCurrentPage().getBitmap()));
}



