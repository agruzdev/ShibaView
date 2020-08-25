/**
 * @file
 *
 * Copyright 2018-2020 Alexey Gruzdev
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

#ifndef IMAGESOURCE_H
#define IMAGESOURCE_H

#include <cassert>
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
    uint16_t offsetX  = 0;
    uint16_t offsetY  = 0;
    uint32_t duration = 0;
    DisposalType disposal = DisposalType::eUnspecified;
};


class ImageSource
{
public:
    ImageSource() = default;
    virtual ~ImageSource() = default;

    ImageSource(const ImageSource&) = delete;
    ImageSource(ImageSource&&) = delete;

    ImageSource& operator=(const ImageSource&) = delete;
    ImageSource& operator=(ImageSource&&) = delete;


    uint32_t pagesCount() const
    {
        return doPagesCount();
    }

    bool storesDiffernece() const
    {
        return doStoresDifference();
    }

    std::shared_ptr<FIBITMAP> lockPage(uint32_t pageIdx, AnimationInfo* anim)
    {
        return std::shared_ptr<FIBITMAP>(doDecodePage(pageIdx, anim), [this](FIBITMAP* page) {
            if (page) {
                doReleasePage(page);
            }
        });
    }

    //---------------------------------------------------------

    static
    std::unique_ptr<ImageSource> Load(const QString& filename) Q_DECL_NOEXCEPT;

protected:

    /**
     * Pages count
     */
    virtual uint32_t doPagesCount() const Q_DECL_NOEXCEPT = 0;

    /**
     * Get page data, read-only mode
     * @param anim - optional output animation info. Image is animated if pagesCount() > 1
     */
    virtual FIBITMAP* doDecodePage(uint32_t pageIdx, AnimationInfo* anim) Q_DECL_NOEXCEPT = 0;

    /**
     * Release page data
     */
    virtual void doReleasePage(FIBITMAP* page) Q_DECL_NOEXCEPT = 0;

    /**
     * Return if pages stores difference
     */
    virtual bool doStoresDifference() const Q_DECL_NOEXCEPT = 0;
};



#endif // IMAGESOURCE_H
