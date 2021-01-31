/**
 * @file
 *
 * Copyright 2018-2021 Alexey Gruzdev
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

#ifndef IMAGEPAGEFLO_H
#define IMAGEPAGEFLO_H

#include "ImagePage.h"

class ImagePageFLO:
    public ImagePage
{
public:
    ImagePageFLO(FIBITMAP* flo, FREE_IMAGE_FORMAT fif);

    ImagePageFLO(const ImagePage&) = delete;

    ImagePageFLO(ImagePageFLO&&) = delete;

    virtual ~ImagePageFLO();

    ImagePageFLO& operator=(const ImagePageFLO&) = delete;

    ImagePageFLO& operator=(ImagePageFLO&&) = delete;

private:
    QString doDescribeFormat() const override;

    bool doGetPixel(uint32_t y, uint32_t x, Pixel* pixel) const override;

    FIBITMAP* mFlowImage;
};

#endif // IMAGEPAGEFLO_H
