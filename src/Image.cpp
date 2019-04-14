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

#include "Image.h"

#include <atomic>
#include <iostream>

#include <QDateTime>
#include <QDebug>
#include <QFileInfo>

#include "ImageSource.h"

namespace
{
    uint64_t generateId()
    {
        static std::atomic<uint64_t> nextId{ 0 };
        return nextId.fetch_add(1, std::memory_order::memory_order_relaxed);
    }
}


FIBITMAP* Image::cvtToInternalType(FIBITMAP* src, QString & srcFormat, bool & dstNeedUnload)
{
    assert(src != nullptr);
    FIBITMAP* dst = nullptr;
    const uint32_t bpp = FreeImage_GetBPP(src);
    mIsHDR = false;
    switch (FreeImage_GetImageType(src)) {
    case FIT_RGBAF:
        srcFormat = "RGBA float";
        dst = src;
        dstNeedUnload = false;
        mIsHDR = true;
        break;

    case FIT_RGBF:
        srcFormat = "RGB float";
        dst = src;
        dstNeedUnload = false;
        mIsHDR = true;
        break;

    case FIT_RGBA16:
        assert(bpp == 64);
        srcFormat = "RGBA16";
        dst = FreeImage_ConvertTo32Bits(src);
        dstNeedUnload = true;
        break;

    case FIT_RGB16:
        assert(bpp == 48);
        srcFormat = "RGB16";
        dst = FreeImage_ConvertTo24Bits(src);
        dstNeedUnload = true;
        break;

    case FIT_UINT16:
        srcFormat = "Greyscale 16bit";
        goto ConvertToStandardType;
    case FIT_INT16:
        srcFormat = "Greyscale 16bit (signed)";
        goto ConvertToStandardType;
    case FIT_UINT32:
        srcFormat = "Greyscale 32bit";
        goto ConvertToStandardType;
    case FIT_INT32:
        srcFormat = "Greyscale 32bit (signed)";
        goto ConvertToStandardType;
    case FIT_FLOAT:
        srcFormat = "Greyscale float";
        goto ConvertToStandardType;
    case FIT_DOUBLE:
        srcFormat = "Greyscale double";

    ConvertToStandardType:
        dst = FreeImage_ConvertToStandardType(src);
        dstNeedUnload = true;
        break;

    case FIT_BITMAP:
        if (32 == bpp) {
            srcFormat = "RGBA8888";
            dst = src;
            dstNeedUnload = false;
        }
        else if (24 == bpp) {
            srcFormat = "RGB888";
            dst = src;
            dstNeedUnload = false;
        }
        else if (8 == bpp) {
            if (FIC_PALETTE == FreeImage_GetColorType(src)) {
                srcFormat = "RGB Indexed 8bit";
                dst = FreeImage_ConvertTo32Bits(src);
                dstNeedUnload = true;
            }
            else {
                srcFormat = "Greyscale 8bit";
                dst = src;
                dstNeedUnload = false;
            }
        }
        else if(4 == bpp) {
            srcFormat = "RGB Indexed 4bit";
            dst = FreeImage_ConvertTo32Bits(src);
            dstNeedUnload = true;
        }
        else if(1 == bpp) {
            srcFormat = "Mono 1bit";
            dst = src;
            dstNeedUnload = false;
        }
        break;

    default:
        break;
    }
    if (dst) {
        FreeImage_FlipVertical(dst);
    }
    return dst;
}

Image::Image(QString name, QString filename) noexcept
    : mId(generateId())
{
#ifdef _MSC_VER
    std::wcout << filename.toStdWString() << std::endl;
#endif

    QString formatInfo = "None";

    // Load bitmap. Keep empty on fail.
    try {
        mImageSource = ImageSource::Load(filename);
        if (!mImageSource || 0 == mImageSource->pagesCount()) {
            throw std::runtime_error("Failed to open image: " + filename.toStdString());
        }

        // Lock first page
        mPageIdx = 0;
        if (!readCurrentPage(formatInfo)) {
            throw std::runtime_error("Failed to read image page");
        }
    }
    catch(std::exception & e) {
        qWarning() << QString(e.what());
        if (mImageSource && mPage) {
            mImageSource->releasePage(mPage);
        }
        mImageSource.reset();
        mBitmapInternal = nullptr;
        mPage = nullptr;
    }

    uint32_t width  = 0;
    uint32_t height = 0;
    if (mBitmapInternal) {
        width  = FreeImage_GetWidth(mBitmapInternal);
        height = FreeImage_GetHeight(mBitmapInternal);
    }

    QFileInfo file(filename);

    mInfo.path     = std::move(filename);
    mInfo.name     = std::move(name);
    mInfo.bytes    = file.size();
    mInfo.format   = formatInfo;
    mInfo.modified = file.lastModified();
    mInfo.dims     = QSize(width, height);

    mInvalidPixmap    = false;
}

Image::~Image()
{
    if (mNeedUnloadBitmap) {
        FreeImage_Unload(mBitmapInternal);
    }
    if (mImageSource && mPage) {
        mImageSource->releasePage(mPage);
    }
}

uint32_t Image::width() const
{
    return !isNull() ? FreeImage_GetWidth(mBitmapInternal) : 0;
}

uint32_t Image::height() const
{
    return !isNull() ? FreeImage_GetHeight(mBitmapInternal) : 0;
}

FIBITMAP* Image::get(PageInfo* info) const
{
    if (!isNull()) {
        if (mInvalidPixmap) {
            QString ignoreFormat;
            mInvalidPixmap = !const_cast<Image*>(this)->readCurrentPage(ignoreFormat);
            //mInvalidTransform = true;
        }
        if (info) {
            *info = mPageInfo;
        }
        return mBitmapInternal;
    }
    return nullptr;
}

uint32_t Image::pagesCount() const Q_DECL_NOEXCEPT
{
    return mImageSource ? mImageSource->pagesCount() : 0;
}

bool Image::readCurrentPage(QString & format) Q_DECL_NOEXCEPT
{
    if (mImageSource) {
        FIBITMAP* prev = nullptr;
        if (mBitmapInternal && mImageSource->storesResidual()) {
            prev = FreeImage_ConvertTo24Bits(mBitmapInternal);
        }

        // Free old internal bitmap, if it owns data
        if (mNeedUnloadBitmap) {
            FreeImage_Unload(mBitmapInternal);
        }
        mBitmapInternal = nullptr;
        // Release old page if it is held
        if (mPage) {
            mImageSource->releasePage(mPage);
            mPage = nullptr;
        }
        // decode current page
        AnimationInfo anim{};
        mPage = mImageSource->decodePage(mPageIdx, &anim);
        if (!mPage) {
            return false;
        }

        // Convert to internal bitmap
        mBitmapInternal = cvtToInternalType(mPage, format, mNeedUnloadBitmap);
        if (!mBitmapInternal) {
            mImageSource->releasePage(mPage);
            return false;
        }

        if (prev) {
            const int32_t dw = FreeImage_GetWidth(prev)  - FreeImage_GetWidth(mBitmapInternal);
            const int32_t dh = FreeImage_GetHeight(prev) - FreeImage_GetHeight(mBitmapInternal);
            if (dw != 0 || dh != 0) {
                // Apply foreground offset
                RGBQUAD transparent {};
                transparent.rgbReserved = 0x00;
                // Internal image is vertically flipped -> offsetY is applied from below
                FIBITMAP* foreground = FreeImage_EnlargeCanvas(mBitmapInternal, anim.offsetX, dh - anim.offsetY, dw - anim.offsetX, anim.offsetY, &transparent, FI_COLOR_IS_RGBA_COLOR);
                if (foreground) {
                    if (mNeedUnloadBitmap) {
                        FreeImage_Unload(mBitmapInternal);
                    }
                    mBitmapInternal = foreground;
                    mNeedUnloadBitmap = true;
                }
            }
            FIBITMAP* composed = FreeImage_Composite(mBitmapInternal, FALSE, nullptr, prev);
            FreeImage_Unload(prev);
            if (composed) {
                if (mNeedUnloadBitmap) {
                    FreeImage_Unload(mBitmapInternal);
                }
                mBitmapInternal = composed;
                mNeedUnloadBitmap = true;
            }
        }

        // Release page early if bitmap owns data
        if (mNeedUnloadBitmap) {
            mImageSource->releasePage(mPage);
            mPage = nullptr;
        }

        mPageInfo.index    = mPageIdx;
        mPageInfo.duration = anim.duration;

        return true;
    }
    return false;
}

void Image::readNextPage() Q_DECL_NOEXCEPT
{
    const uint32_t count = pagesCount();
    if (count > 1) {
        mPageIdx = mPageIdx + 1;
        if(mPageIdx >= count) {
            mPageIdx = 0;
        }
        mInvalidPixmap = true;

        for (auto & listener : mListeners) {
            listener->onInvalidated(this);
        }
    }
}

void Image::addListener(ImageListener* listener)
{
    mListeners.push_back(listener);
}

void Image::removeListener(ImageListener* listener)
{
    const auto it = std::find(mListeners.cbegin(), mListeners.cend(), listener);
    if (it != mListeners.cend()) {
        mListeners.erase(it);
    }
}

