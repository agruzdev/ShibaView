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

#ifndef IMAGE_H
#define IMAGE_H

#include <limits>
#include <memory>

#include <QPixmap>

#include "ImageInfo.h"

#include "FreeImage.h"

class ImageSource;

static Q_CONSTEXPR uint32_t kNoneIndex = std::numeric_limits<uint32_t>::max();

struct Frame
{
    QPixmap pixmap;
    uint32_t pageIndex = kNoneIndex;
    uint32_t duration  = 0;
};

class Image;

class ImageListener
{
public:
    virtual ~ImageListener() = default;

    virtual void onInvalidated(Image*) { };
};

class Image
{
    using BitmapPtr = std::unique_ptr<FIBITMAP, decltype(&::FreeImage_Unload)>;

public:
    static Q_CONSTEXPR uint32_t kNonePage = std::numeric_limits<uint32_t>::max();

    struct PageInfo
    {
        uint32_t index    = kNonePage;
        uint32_t duration = 0;
    };

    Image(QString name, QString filename) noexcept;
    ~Image();

    Image(const Image&) = delete;
    Image(Image&&) = delete;

    Image& operator=(const Image&) = delete;
    Image& operator=(Image&&) = delete;

    FIBITMAP* get(PageInfo* info) const;

    const ImageInfo & info() const
    {
        return mInfo;
    }

    uint32_t width() const;
    uint32_t height() const;

    bool isNull() const
    {
        return (mBitmapInternal == nullptr);
    }

    uint32_t pagesCount() const Q_DECL_NOEXCEPT;

    void readNextPage() Q_DECL_NOEXCEPT;

    uint64_t id() const
    {
        return mId;
    }

    /**
     * True is pixel is floating point
     */
    bool isHDR() const
    {
        return mIsHDR;
    }

    /**
     * True for color images
     */
    bool isRGB() const
    {
        return mIsRGB;
    }

    void addListener(ImageListener* listener);
    void removeListener(ImageListener* listener);

private:

    FIBITMAP* cvtToInternalType(FIBITMAP* src, QString & srcFormat, bool & dstNeedUnload);
    bool readCurrentPage(QString & format) Q_DECL_NOEXCEPT;

    uint64_t mId;

    std::unique_ptr<ImageSource> mImageSource;
    ImageInfo mInfo;

    uint32_t mPageIdx = kNonePage;
    FIBITMAP* mPage = nullptr;

    FIBITMAP* mBitmapInternal = nullptr;
    bool mNeedUnloadBitmap = false;

    mutable bool mInvalidPixmap = false;

    mutable PageInfo mPageInfo;

    bool mIsHDR = false;
    bool mIsRGB = false;

    std::vector<ImageListener*> mListeners;
};

using ImagePtr = QSharedPointer<Image>;

#endif // IMAGE_H
