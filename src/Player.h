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

#include "FreeImage.h"
#include <QString>

#include "ImageSource.h"
#include "Pixel.h"

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


struct ImageFrame
{
    FIBITMAP* bmp = nullptr;
    uint32_t index = kNoneIndex;
    uint32_t duration = 0;
    FrameFlags flags = FrameFlags::eNone;
    const ImagePage* page = nullptr; // not null
};


class Player
{
public:
    Player(std::shared_ptr<ImageSource> src);

    Player(const Player&) = delete;

    Player(Player&&) = delete;

    ~Player();

    Player& operator=(const Player&) = delete;

    Player& operator=(Player&&) = delete;

    const ImageFrame & getCurrentFrame() const;

    uint32_t framesNumber() const;

    uint32_t getWidth() const;

    uint32_t getHeight() const;

    void next();

    void prev();

    bool getPixel(uint32_t y, uint32_t x, Pixel* p) const;

    //-------------------------------------------------------------------------------------

    static
    ImageFrame cvtToInternalType(FIBITMAP* src, bool & dstNeedUnload);

    static
    UniqueBitmap getOrMakeThumbnail(FIBITMAP* src, uint32_t maxSize);

private:
    struct ConvertionContext;

    //-------------------------------------------------------------------------------------

    std::unique_ptr<ConvertionContext> loadZeroFrame(ImageSource* source);

    std::unique_ptr<ConvertionContext> loadNextFrame(ImageSource* source, const ConvertionContext* prev);

    ImageFrame* getImpl() const;
    //-------------------------------------------------------------------------------------

    std::shared_ptr<ImageSource> mSource;

    std::deque<std::unique_ptr<ConvertionContext>> mFramesCache;
    size_t mCacheIndex   = 0;
    size_t mMaxCacheSize = 1;
};

#endif // PLAYER_H