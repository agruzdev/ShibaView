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

#ifndef IMAGE_H
#define IMAGE_H

#include <cassert>
#include <limits>
#include <memory>

#include <QPixmap>

#include "ImageInfo.h"
#include "Player.h"
#include "FreeImage.h"

class Image;
class ImagePage;

class ImageListener
{
public:
    virtual ~ImageListener() = default;

    virtual void onInvalidated(Image*) { };
};

class Image
{
public:
    Image(QString name, QString filename) noexcept;

    Image(const Image&) = delete;

    Image(Image&&) = delete;

    ~Image();

    Image& operator=(const Image&) = delete;

    Image& operator=(Image&&) = delete;

    const ImageFrame& currentFrame() const;

    const ImageInfo& info() const
    {
        return mInfo;
    }

    bool getPixel(uint32_t y, uint32_t x, Pixel* p) const
    {
        if (mImagePlayer) {
            return mImagePlayer->getPixel(y, x, p);
        }
        return false;
    }

    uint32_t width() const
    {
        return mInfo.dims.width;
    }

    uint32_t height() const
    {
        return mInfo.dims.height;
    }

    uint32_t channels() const;

    bool isNull() const
    {
        return (mImagePlayer == nullptr);
    }

    bool notNull() const
    {
        return (mImagePlayer != nullptr);
    }

    uint32_t pagesCount() const
    {
        return mImagePlayer ? mImagePlayer->framesNumber() : 0;
    }

    /**
     * Current page access
     */
    const ImagePage& currentPage() const;

    /**
     * Scroll to next page
     */
    void next();

    /**
     * Scroll to previous page
     */
    void prev();

    uint64_t id() const
    {
        return mId;
    }

    void addListener(ImageListener* listener);

    void removeListener(ImageListener* listener);

private:
    uint64_t mId;

    std::unique_ptr<Player> mImagePlayer;

    ImageInfo mInfo;

    std::vector<ImageListener*> mListeners;
};

using ImagePtr = QSharedPointer<Image>;

#endif // IMAGE_H
