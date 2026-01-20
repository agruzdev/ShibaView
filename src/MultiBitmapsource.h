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

#ifndef MULTIBITMAPSOURCE_H
#define MULTIBITMAPSOURCE_H

#include "ImageSource.h"
#include <QString>


class MultibitmapSource
    : public ImageSource
{
public:
    MultibitmapSource(const QString & filename, FREE_IMAGE_FORMAT fif);

    MultibitmapSource(const MultibitmapSource&) = delete;

    MultibitmapSource(MultibitmapSource&&) = delete;

    ~MultibitmapSource() Q_DECL_OVERRIDE;

    MultibitmapSource & operator=(const MultibitmapSource&) = delete;

    MultibitmapSource & operator=(MultibitmapSource&&) = delete;

private:
    uint32_t doPagesCount() const Q_DECL_OVERRIDE;

    const ImagePage* doDecodePage(uint32_t pageIdx) Q_DECL_OVERRIDE;

    void doReleasePage(const ImagePage* page) Q_DECL_OVERRIDE;

    bool doStoresDifference() const Q_DECL_OVERRIDE;

    FREE_IMAGE_FORMAT doGetFormat() const Q_DECL_OVERRIDE;


    FREE_IMAGE_FORMAT mImageFormat;
    FIMULTIBITMAP* mMultibitmap = nullptr;
};

#endif // MULTIBITMAPSOURCE_H
