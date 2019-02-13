/**
* ShibaView
*
* The MIT License (MIT)
* Copyright (c) 2018 Alexey Gruzdev
*/

#ifndef MULTIBITMAPSOURCE_H
#define MULTIBITMAPSOURCE_H

#include "ImageSource.h"

#include <QString>

struct MultibitmapBuffer;

class MultibitmapSource
    : public ImageSource
{
public:
    MultibitmapSource(const QString & filename, FREE_IMAGE_FORMAT fif);
    ~MultibitmapSource() Q_DECL_OVERRIDE;

    MultibitmapSource(const MultibitmapSource&) = delete;
    MultibitmapSource(MultibitmapSource&&) = delete;

    MultibitmapSource & operator=(const MultibitmapSource&) = delete;
    MultibitmapSource & operator=(MultibitmapSource&&) = delete;

    /**
     * Pages count
     */
    uint32_t pagesCount() const Q_DECL_NOEXCEPT Q_DECL_OVERRIDE;

    /**
     * Get page data, read-only mode
     */
    FIBITMAP* decodePage(uint32_t pageIdx, AnimationInfo* anim) Q_DECL_NOEXCEPT Q_DECL_OVERRIDE;

    /**
     * Release page data
     */
    void releasePage(FIBITMAP* page) Q_DECL_NOEXCEPT Q_DECL_OVERRIDE;

    /**
     * Return if pages store only residual map
     */
    bool storesResidual() const Q_DECL_NOEXCEPT Q_DECL_OVERRIDE;

private:
    FREE_IMAGE_FORMAT mFormat;
    FIMULTIBITMAP* mMultibitmap = nullptr;
    std::unique_ptr<MultibitmapBuffer> mBuffer = nullptr;
};

#endif // MULTIBITMAPSOURCE_H
