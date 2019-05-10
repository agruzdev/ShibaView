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

#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <QSharedPointer>
#include <QPixmap>

#include "FreeImageExt.h"
#include "Image.h"

enum class Rotation
{
    eDegree0   = 0,
    eDegree90  = 1,
    eDegree180 = 2,
    eDegree270 = 3,

    length_ = 4
};

constexpr
int toDegree(Rotation r)
{
    return 90 * static_cast<int>(r);
}


class ImageProcessor
    : public ImageListener
{
public:
    ImageProcessor();
    ~ImageProcessor();

    ImageProcessor(QWeakPointer<Image> image)
        : ImageProcessor()
    {
        attachSource(image);
    }

    ImageProcessor(const ImageProcessor&) = delete;
    ImageProcessor(ImageProcessor&&) = delete;

    ImageProcessor& operator=(const ImageProcessor&) = delete;
    ImageProcessor& operator=(ImageProcessor&&) = delete;

    void attachSource(QWeakPointer<Image> image);

    void detachSource();

    Rotation rotation() const
    {
        return mRotation;
    }
    
    void setRotation(Rotation r)
    {
        if (mRotation != r) {
            mRotation = r;
            mIsValid = false;
        }
    }

    FIE_ToneMapping toneMappingMode() const
    {
        return mToneMapping;
    }

    void setToneMappingMode(FIE_ToneMapping mode)
    {
        if(mToneMapping != mode) {
            mToneMapping = mode;
            mIsValid = false;
        }
    }

    /**
     * Image width after processing
     */
    uint32_t width() const;

    /**
     * Image height after processing
     */
    uint32_t height() const;

    /**
     * Processed frame, ready to draw
     */
    const QPixmap & getResult();

private:
    void onInvalidated(Image* emitter) override;

private:
    QWeakPointer<Image> mSrcImage;
    QPixmap mDstPixmap;

    bool mIsValid = false;

    Rotation mRotation = Rotation::eDegree0;
    FIE_ToneMapping mToneMapping = FIE_ToneMapping::FIETMO_NONE;
};

#endif // IMAGEPROCESSOR_H
