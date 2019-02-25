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
