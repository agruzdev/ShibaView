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

#ifndef MULTIBITMAPSOURCE_H
#define MULTIBITMAPSOURCE_H

#include "ImageSource.h"

#include <QString>
#include <vector>

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
    uint32_t doPagesCount() const Q_DECL_NOEXCEPT Q_DECL_OVERRIDE;

    /**
     * Get page data, read-only mode
     */
    FIBITMAP* doDecodePage(uint32_t pageIdx, AnimationInfo* anim) Q_DECL_NOEXCEPT Q_DECL_OVERRIDE;

    /**
     * Release page data
     */
    void doReleasePage(FIBITMAP* page) Q_DECL_NOEXCEPT Q_DECL_OVERRIDE;

    /**
     * Return if pages store only residual map
     */
    bool doStoresDifference() const Q_DECL_NOEXCEPT Q_DECL_OVERRIDE;

private:
    // Internal buffer for FreeImage_OpenMultiBitmapU issue workaround
    struct MultibitmapBuffer
    {
        std::vector<unsigned char> data;
        FIMEMORY* stream = nullptr;
    };

    FREE_IMAGE_FORMAT mFormat;
    FIMULTIBITMAP* mMultibitmap = nullptr;
    std::unique_ptr<MultibitmapBuffer> mBuffer = nullptr;
};

#endif // MULTIBITMAPSOURCE_H
