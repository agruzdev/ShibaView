/**
* ShibaView
*
* The MIT License (MIT)
* Copyright (c) 2018 Alexey Gruzdev
*/

#ifndef IMAGE_H
#define IMAGE_H

#include <limits>
#include <memory>

#include <QPixmap>

#include "ImageInfo.h"

#include "FreeImage.h"

enum class Rotation
{
    eDegree0   = 0,
    eDegree90  = 90,
    eDegree180 = 180,
    eDegree270 = 270
};

class ImageSource;

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

    const QPixmap & pixmap(PageInfo* info) const;

    const ImageInfo & info() const
    {
        return mInfo;
    }

    uint32_t width() const;
    uint32_t height() const;

    uint32_t sourceWidth() const;
    uint32_t sourceHeight() const;

    bool isNull() const
    {
        return (mBitmapInternal != nullptr);
    }

    void setRotation(Rotation r)
    {
        if (mRotation != r) {
            mRotation = r;
            mInvalidTransform = true;
        }
    }

    uint32_t pagesCount() const Q_DECL_NOEXCEPT;

    void readNextPage() Q_DECL_NOEXCEPT;

    uint64_t id() const
    {
        return mId;
    }

private:

    bool readCurrentPage(QString & format) Q_DECL_NOEXCEPT;
    QPixmap recalculatePixmap() const;

    uint64_t mId;

    std::unique_ptr<ImageSource> mImageSource;
    ImageInfo mInfo;

    uint32_t mPageIdx = kNonePage;
    FIBITMAP* mPage = nullptr;

    FIBITMAP* mBitmapInternal = nullptr;
    bool mNeedUnloadBitmap = false;

    mutable QPixmap mPixmap;
    mutable bool mInvalidPixmap = false;

    mutable bool mInvalidTransform = false;
    Rotation mRotation = Rotation::eDegree0;

    mutable PageInfo mPageInfo;
};

using ImagePtr = QSharedPointer<Image>;

#endif // IMAGE_H
