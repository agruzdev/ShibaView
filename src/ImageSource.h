/**
* ShibaView
*
* The MIT License (MIT)
* Copyright (c) 2018 Alexey Gruzdev
*/

#ifndef IMAGESOURCE_H
#define IMAGESOURCE_H

#include <cstdint>
#include <memory>

#include <QString>

#include "FreeImage.h"

enum class DisposalType
    : uint8_t
{
    eUnspecified = 0,
    eLeave = 1,
    eBackground = 2,
    ePrevious = 3
};

struct AnimationInfo
{
    uint16_t offsetX;
    uint16_t offsetY;
    uint32_t duration;
    DisposalType disposal;
};

class ImageSource
{
public:
    virtual ~ImageSource() = default;

    /**
     * Pages count
     */
    virtual uint32_t pagesCount() const Q_DECL_NOEXCEPT = 0;

    /**
     * Get page data, read-only mode
     * @param anim - optional output animation info. Image is animated if pagesCount() > 1
     */
    virtual FIBITMAP* decodePage(uint32_t pageIdx, AnimationInfo* anim) Q_DECL_NOEXCEPT = 0;

    /**
     * Release page data
     */
    virtual void releasePage(FIBITMAP* page) Q_DECL_NOEXCEPT = 0;

    /**
     * Return if pages store only residual map
     */
    virtual bool storesResidual() const = 0;

    //---------------------------------------------------------

    static
    std::unique_ptr<ImageSource> Load(const QString & filename) Q_DECL_NOEXCEPT;
};



#endif // IMAGESOURCE_H
