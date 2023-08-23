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

#include "MultiBitmapsource.h"

#include <stdio.h>
#include <limits>
#include <vector>
#include <fstream>
#include "FreeImageExt.h"


MultibitmapSource::MultibitmapSource(const QString & filename, FREE_IMAGE_FORMAT fif)
    : mImageFormat(fif)
{
#ifdef _WIN32
    const auto uniName = filename.toStdWString();
    // ToDo: Since FreeImage is missing FreeImage_OpenMultiBitmapU function, read through memory buffer
# ifdef _MSC_VER
    FILE* file{ nullptr };
    _wfopen_s(&file, uniName.c_str(), L"rb");
# else
    FILE* file = _wfopen(uniName.c_str(), L"rb");
# endif
    if (file) {
        mBuffer = std::make_unique<MultibitmapBuffer>();

        fseek(file, 0, SEEK_END);
        mBuffer->data.resize(ftell(file));
        fseek(file, 0, SEEK_SET);
        fread(mBuffer->data.data(), sizeof(unsigned char), mBuffer->data.size(), file);
        fclose(file);

        if(mBuffer->data.size() > std::numeric_limits<uint32_t>::max()) {
            throw std::runtime_error("MultibitmapSource[MultibitmapSource]: Input file is too big!");
        }

        mBuffer->stream = FreeImage_OpenMemory(mBuffer->data.data(), static_cast<uint32_t>(mBuffer->data.size()));
        if (mBuffer->stream) {
            mMultibitmap = FreeImage_LoadMultiBitmapFromMemory(mImageFormat, mBuffer->stream, JPEG_EXIFROTATE);
        }
    }
#else
    const auto utfName = filename.toUtf8().toStdString();
    mMultibitmap = FreeImage_OpenMultiBitmap(fif, utfName.c_str(), FALSE, TRUE, FALSE, JPEG_EXIFROTATE);
#endif
    if (nullptr == mMultibitmap) {
        throw std::runtime_error("MultibitmapSource[MultibitmapSource]: Failed to load file.");
    }
}

MultibitmapSource::~MultibitmapSource()
{
    FreeImage_CloseMultiBitmap(mMultibitmap);
    if (mBuffer && mBuffer->stream) {
        FreeImage_CloseMemory(mBuffer->stream);
    }
}

uint32_t MultibitmapSource::doPagesCount() const
{
    return static_cast<uint32_t>(FreeImage_GetPageCount(mMultibitmap));
}

const ImagePage* MultibitmapSource::doDecodePage(uint32_t pageIdx)
{
    auto page = std::make_unique<ImagePage>(FreeImage_LockPage(mMultibitmap, static_cast<int>(pageIdx)), pageIdx);
    AnimationInfo anim{};
    anim.offsetX  = FreeImageExt_GetMetadataValue(FIMD_ANIMATION, page->getSourceBitmap(), "FrameLeft", 0);
    anim.offsetY  = FreeImageExt_GetMetadataValue(FIMD_ANIMATION, page->getSourceBitmap(), "FrameTop",  0);
    anim.duration = FreeImageExt_GetMetadataValue(FIMD_ANIMATION, page->getSourceBitmap(), "FrameTime", 0);
    anim.disposal = FreeImageExt_GetMetadataValue(FIMD_ANIMATION, page->getSourceBitmap(), "DisposalMethod", DisposalType::eLeave);
    page->setAnimation(std::move(anim));
    return page.release();
}

void MultibitmapSource::doReleasePage(const ImagePage* page)
{
    if (page) {
        FreeImage_UnlockPage(mMultibitmap, page->getSourceBitmap(), false);
    }
    delete page;
}

bool MultibitmapSource::doStoresDifference() const
{
    return (mImageFormat == FIF_GIF);
}
