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
#include "FreeImageExt.h"


MultibitmapSource::MultibitmapSource(const QString & filename, FREE_IMAGE_FORMAT fif)
    : mImageFormat(fif)
{
    int loadFlags = 0;
    if (mImageFormat == FIF_ICO) {
        loadFlags = ICO_MAKEALPHA;  // load all pages with transparency
    }

#ifdef _WIN32
    const auto uniName = filename.toStdWString();
    mMultibitmap = FreeImage_OpenMultiBitmapU(static_cast<FREE_IMAGE_FORMAT>(fif), uniName.c_str(), FALSE, TRUE, FALSE, loadFlags);
#else
    const auto utfName = filename.toUtf8().toStdString();
    mMultibitmap = FreeImage_OpenMultiBitmap(static_cast<FREE_IMAGE_FORMAT>(fif), utfName.c_str(), FALSE, TRUE, FALSE, loadFlags);
#endif
    if (nullptr == mMultibitmap) {
        throw std::runtime_error("MultibitmapSource[MultibitmapSource]: Failed to load file.");
    }
}

MultibitmapSource::~MultibitmapSource()
{
    FreeImage_CloseMultiBitmap(mMultibitmap);
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

FREE_IMAGE_FORMAT MultibitmapSource::doGetFormat() const
{
    return mImageFormat;
}
