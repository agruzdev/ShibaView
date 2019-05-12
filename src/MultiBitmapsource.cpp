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

#include "MultiBitmapsource.h"

#include <stdio.h>
#include <limits>
#include <vector>
#include <fstream>


namespace
{
    template <typename Ty_>
    Ty_ FreeImage_GetMetadataValue(FREE_IMAGE_MDMODEL model, FIBITMAP *dib, const char *key, const Ty_ & defaultVal)
    {
        FITAG* tag = nullptr;
        const auto succ = FreeImage_GetMetadata(model, dib, key, &tag);
        if(succ && tag) {
            return *static_cast<std::add_const_t<Ty_>*>(FreeImage_GetTagValue(tag));
        }
        return defaultVal;
    }
}



MultibitmapSource::MultibitmapSource(const QString & filename, FREE_IMAGE_FORMAT fif)
    : mFormat(fif)
{
#ifdef _WIN32
    const auto uniName = filename.toStdWString();
    // ToDo: Since FreeImage is missing FreeImage_OpenMultiBitmapU function, read through memory buffer
    FILE* file = _wfopen(uniName.c_str(), L"rb");
    if (file) {
        mBuffer = std::make_unique<MultibitmapBuffer>();

        fseek(file, 0, SEEK_END);
        mBuffer->data.resize(ftell(file));
        fseek(file, 0, SEEK_SET);
        fread(mBuffer->data.data(), sizeof(unsigned char), mBuffer->data.size(), file);
        fclose(file);

        if(mBuffer->data.size() > std::numeric_limits<DWORD>::max()) {
            throw std::runtime_error("MultibitmapSource[MultibitmapSource]: Input file is too big!");
        }

        mBuffer->stream = FreeImage_OpenMemory(mBuffer->data.data(), static_cast<DWORD>(mBuffer->data.size()));
        if (mBuffer->stream) {
            mMultibitmap = FreeImage_LoadMultiBitmapFromMemory(fif, mBuffer->stream, JPEG_EXIFROTATE);
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

uint32_t MultibitmapSource::doPagesCount() const Q_DECL_NOEXCEPT
{
    return static_cast<uint32_t>(FreeImage_GetPageCount(mMultibitmap));
}

FIBITMAP* MultibitmapSource::doDecodePage(uint32_t pageIdx, AnimationInfo* anim) Q_DECL_NOEXCEPT
{
    const auto bmp = FreeImage_LockPage(mMultibitmap, static_cast<int>(pageIdx));
    if (bmp && anim) {
        anim->offsetX  = FreeImage_GetMetadataValue(FIMD_ANIMATION, bmp, "FrameLeft", 0);
        anim->offsetY  = FreeImage_GetMetadataValue(FIMD_ANIMATION, bmp, "FrameTop",  0);
        anim->duration = FreeImage_GetMetadataValue(FIMD_ANIMATION, bmp, "FrameTime", 0);
        anim->disposal = FreeImage_GetMetadataValue(FIMD_ANIMATION, bmp, "DisposalMethod", DisposalType::eLeave);
    }
    return bmp;
}

void MultibitmapSource::doReleasePage(FIBITMAP* page) Q_DECL_NOEXCEPT
{
    FreeImage_UnlockPage(mMultibitmap, page, false);
}

bool MultibitmapSource::doStoresDifference() const Q_DECL_NOEXCEPT
{
    return mFormat == FIF_GIF;
}
