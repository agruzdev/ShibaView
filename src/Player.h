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

#ifndef PLAYER_H
#define PLAYER_H

#include <deque>
#include <type_traits>
#include <memory>
#include <deque>
#include "FreeImageExt.h"

class ImageSource;
class ImagePage;
class ImageFrame;
class Pixel;

class Player
{
public:
    Player(std::shared_ptr<ImageSource> src);

    Player(const Player&) = delete;

    Player(Player&&) = delete;

    ~Player();

    Player& operator=(const Player&) = delete;

    Player& operator=(Player&&) = delete;

    const ImageFrame& getCurrentFrame() const;

    const ImagePage& getCurrentPage() const;

    FIBITMAP* getBlendedBitmap() const;

    uint32_t framesNumber() const;

    uint32_t getWidth() const;

    uint32_t getHeight() const;

    void next();

    void prev();

    bool getPixel(uint32_t y, uint32_t x, Pixel* p) const;

private:
    struct CacheEntry;

    //-------------------------------------------------------------------------------------

    std::unique_ptr<CacheEntry> loadZeroFrame(ImageSource* source);

    std::unique_ptr<CacheEntry> loadNextFrame(ImageSource* source, const CacheEntry& prev);

    //-------------------------------------------------------------------------------------

    std::shared_ptr<ImageSource> mSource;

    std::deque<std::unique_ptr<CacheEntry>> mFramesCache;
    size_t mCacheIndex   = 0;
    size_t mMaxCacheSize = 1;
};

#endif // PLAYER_H