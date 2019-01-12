#ifndef IMAGE_H
#define IMAGE_H

#include <memory>

#include <QPixmap>
#include "ImageInfo.h"

#include "FreeImage.h"

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

    uint32_t width() const
    {
        return mInfo.dims.width();
    }

    uint32_t height() const
    {
        return mInfo.dims.height();
    }

    bool isValid() const
    {
        return (mBitmap != nullptr);
    }

private:
    BitmapPtr mBitmap{ nullptr, &::FreeImage_Unload };
    ImageInfo mInfo;

    QPixmap mPixmap;
};

using ImagePtr = QSharedPointer<Image>;

#endif // IMAGE_H
