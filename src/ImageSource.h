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

#ifndef IMAGESOURCE_H
#define IMAGESOURCE_H

#include <cassert>
#include <cstdint>
#include <memory>

#include "FreeImage.h"
#include "ImagePage.h"

#include <QString>


class ImageSource:
    public std::enable_shared_from_this<ImageSource>
{
private:
    class ImagePageDeleter
    {
    public:
        ImagePageDeleter(const std::shared_ptr<ImageSource>& parent)
            : mParent(parent)
        {
            assert(parent != nullptr);
        }

        ImagePageDeleter(const ImagePageDeleter&) = default;

        ImagePageDeleter(ImagePageDeleter&&) = default;

        ~ImagePageDeleter() = default;

        ImagePageDeleter& operator=(const ImagePageDeleter&) = default;

        ImagePageDeleter& operator=(ImagePageDeleter&&) = default;

        void operator()(const ImagePage* page) const {
            if (page) {
                if (const auto parentPtr = mParent.lock()) {
                    parentPtr->doReleasePage(page);
                }
            }
        }
    private:
        std::weak_ptr<ImageSource> mParent;
    };

public:
    using ImagePagePtr = std::unique_ptr<const ImagePage, ImagePageDeleter>;

    ImageSource(const ImageSource&) = delete;

    ImageSource(ImageSource&&) = delete;

    virtual ~ImageSource() = default;

    ImageSource& operator=(const ImageSource&) = delete;

    ImageSource& operator=(ImageSource&&) = delete;

    uint32_t pagesCount() const
    {
        return doPagesCount();
    }

    bool storesDifference() const
    {
        return doStoresDifference();
    }

    FREE_IMAGE_FORMAT getFormat() const
    {
        return doGetFormat();
    }

    ImagePagePtr lockPage(uint32_t pageIdx)
    {
        return ImagePagePtr(doDecodePage(pageIdx), ImagePageDeleter(shared_from_this()));
    }

    //---------------------------------------------------------

    static
    std::shared_ptr<ImageSource> Load(const QString& filename) Q_DECL_NOEXCEPT;

    static
    void Save(FIBITMAP* bmp, const QString& filename);

protected:
    ImageSource() = default;

    /**
     * Pages count
     */
    virtual uint32_t doPagesCount() const = 0;

    /**
     * Get page data, read-only mode
     */
    virtual const ImagePage* doDecodePage(uint32_t pageIdx) = 0;

    /**
     * Release page data
     */
    virtual void doReleasePage(const ImagePage* page) = 0;

    /**
     * Return if pages stores difference
     */
    virtual bool doStoresDifference() const = 0;

    /**
     * Return original source format
     */
    virtual FREE_IMAGE_FORMAT doGetFormat() const = 0;
};



#endif // IMAGESOURCE_H
