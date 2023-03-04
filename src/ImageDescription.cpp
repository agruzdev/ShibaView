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

#include "ImageDescription.h"
#include "Global.h"

namespace
{
    constexpr size_t kMaxPathLength = 128;

    inline
    QString toPercent(float z)
    {
        return QString::number(100.0f * z, 'f', 0) + QString("%");
    }
}

QVector<QString> ImageDescription::toLines() const
{
    QVector<QString> res;
    const QString& name = mDisplayPath ? mFileInfo.path : mFileInfo.name;
    if (name.length() < kMaxPathLength) {
        res.push_back("File name: " + name);
    }
    else {
        res.push_back("File name: ..." + name.right(kMaxPathLength));
    }
    if (mChangedFlag) {
        res.back().append("*");
    }
    if (mImagesCount > 0 && mImageIndex < mImagesCount) {
        res.back().append(" [" + QString::number(mImageIndex + 1) + '/' + QString::number(mImagesCount) + "]");
    }
    else {
        res.back().append(" [?]");
    }
    res.push_back("File size: " + QString::number(mFileInfo.bytes / 1024.0f, 'f', 1) + "KB");
    res.push_back("Format: " + mFormat);
    if (mToneMapping != FIE_ToneMapping::FIETMO_NONE) {
        res.back().append(" (TM: " + QString::fromUtf8(FreeImageExt_TMtoString(mToneMapping)) + ")");
    }
    if (mGammaValue != 1.0) {
        res.back().append(" (" + QString::fromUtf8(UTF8_GAMMA) + ": " + QString::number(mGammaValue, 'f', 2) + ")");
    }
    res.push_back("Last modified: " + mFileInfo.modified.toString("yyyy/MM/dd hh:mm:ss"));
    res.push_back("Resolution: " + QString::number(mFileInfo.dims.width) + "x" + QString::number(mFileInfo.dims.height));
    res.push_back("");
    res.push_back("Zoom: " + toPercent(mZoomFactor));
    return res;
}


