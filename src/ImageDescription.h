/**
 * @file
 *
 * Copyright 2018-2026 Alexey Gruzdev
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

#ifndef IMAGEDESCRIPTION_H
#define IMAGEDESCRIPTION_H

#include <QVector>

#include "ImageInfo.h"
#include "FreeImageExt.h"

class ImageDescription
{
public:
    ImageDescription()
    { }

    void setImageInfo(ImageInfo info)
    {
        mFileInfo = std::move(info);
    }

    void setFormat(QString fmt)
    {
        mFormat = std::move(fmt);
    }

    void setToneMapping(FREE_IMAGE_TMO mode)
    {
        mToneMapping = mode;
    }

    void setGammaValue(double value)
    {
        mGammaValue = value;
    }

    void setZoom(float factor)
    {
        mZoomFactor = factor;
    }

    void setChanged(bool flag)
    {
        mChangedFlag = flag;
    }

    void setImageIndex(size_t idx, size_t count)
    {
        mImageIndex = idx;
        mImagesCount = count;
    }

    void setErrors(QStringList err)
    {
        mErrors = std::move(err);
    }

    QVector<QString> toLines(bool fullPath = false) const;


private:
    ImageInfo mFileInfo;
    QString mFormat;
    float mZoomFactor = 1.0f;
    FREE_IMAGE_TMO mToneMapping = FITMO_CLAMP;
    double mGammaValue = 1.0;
    bool mChangedFlag = false;
    size_t mImageIndex = 0;
    size_t mImagesCount = 0;
    QStringList mErrors;
};

#endif // IMAGEDESCRIPTION_H
