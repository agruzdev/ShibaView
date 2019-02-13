/**
* ShibaView
*
* The MIT License (MIT)
* Copyright (c) 2018 Alexey Gruzdev
*/

#ifndef BITMAPSOURCE_H
#define BITMAPSOURCE_H

#include <QString>

#include "ImageSource.h"

class BitmapSource 
    : public ImageSource
{
public:
    BitmapSource(const QString & filename, FREE_IMAGE_FORMAT fif);
    ~BitmapSource() Q_DECL_OVERRIDE;

    BitmapSource(const BitmapSource&) = delete;
    BitmapSource(BitmapSource&&) = delete;

    BitmapSource & operator=(const BitmapSource&) = delete;
    BitmapSource & operator=(BitmapSource&&) = delete;

    /**
     * Pages count
     */
    uint32_t pagesCount() const Q_DECL_NOEXCEPT Q_DECL_OVERRIDE;

    /**
     * Get page data, read-only mode
     */
    FIBITMAP* decodePage(uint32_t page, AnimationInfo* anim) Q_DECL_NOEXCEPT Q_DECL_OVERRIDE;

    /**
     * Release page data
     */
    void releasePage(FIBITMAP*) Q_DECL_NOEXCEPT Q_DECL_OVERRIDE;

    /**
     * Return if pages store only residual map
     */
    bool storesResidual() const Q_DECL_NOEXCEPT Q_DECL_OVERRIDE;

private:
    FIBITMAP* mBitmap;
};

#endif // BITMAPSOURCE_H
