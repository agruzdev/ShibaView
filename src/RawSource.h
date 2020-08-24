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

#ifndef RAWSOURCE_H
#define RAWSOURCE_H

#include <QString>
#include "ImageSource.h"

class RawSource 
    : public ImageSource
{
public:
    RawSource(const QString & filename);
    ~RawSource() Q_DECL_OVERRIDE;

    RawSource(const RawSource&) = delete;
    RawSource(RawSource&&) = delete;

    RawSource & operator=(const RawSource&) = delete;
    RawSource & operator=(RawSource&&) = delete;

    /**
     * Pages count
     */
    uint32_t doPagesCount() const Q_DECL_NOEXCEPT Q_DECL_OVERRIDE;

    /**
     * Get page data, read-only mode
     */
    FIBITMAP* doDecodePage(uint32_t page, AnimationInfo* anim) Q_DECL_NOEXCEPT Q_DECL_OVERRIDE;

    /**
     * Release page data
     */
    void doReleasePage(FIBITMAP*) Q_DECL_NOEXCEPT Q_DECL_OVERRIDE;

    /**
     * Return if pages store only difference
     */
    bool doStoresDifference() const Q_DECL_NOEXCEPT Q_DECL_OVERRIDE;

private:
    FIBITMAP* mBitmap;
};

#endif // RAWSOURCE_H
