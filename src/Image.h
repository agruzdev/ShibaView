/**
* ShibaView
*
* The MIT License (MIT)
* Copyright (c) 2018 Alexey Gruzdev
*/

#ifndef IMAGE_H
#define IMAGE_H

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
    Image(QString name, QString filename) noexcept;
    ~Image();

    Image(const Image&) = delete;
    Image(Image&&) = delete;

    Image& operator=(const Image&) = delete;
    Image& operator=(Image&&) = delete;

    const QPixmap & pixmap() const;

    const ImageInfo & info() const
    {
        return mInfo;
    }

    uint32_t width() const;
    uint32_t height() const;

    uint32_t sourceWidth() const;
    uint32_t sourceHeight() const;

    bool isValid() const
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

    uint32_t pageIdx() const
    {
        return mPageIdx;
    }

    uint32_t pagesCount() const Q_DECL_NOEXCEPT;

    void setPageIdx(uint32_t idx) Q_DECL_NOEXCEPT;

private:

    bool readCurrentPage(QString & format) Q_DECL_NOEXCEPT;
    QPixmap recalculatePixmap() const;

    std::unique_ptr<ImageSource> mImageSource;
    ImageInfo mInfo;

    uint32_t mPageIdx = 0;
    FIBITMAP* mPage = nullptr;

    FIBITMAP* mBitmapInternal = nullptr;
    bool mNeedUnloadBitmap = false;

    mutable QPixmap mPixmap;
    mutable bool mInvalidPixmap = false;

    mutable bool mInvalidTransform = false;
    Rotation mRotation = Rotation::eDegree0;
};

using ImagePtr = QSharedPointer<Image>;

#endif // IMAGE_H
