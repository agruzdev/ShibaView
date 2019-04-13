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
    float clamp(float x, float lo = 0.0f, float hi = 1.0f)
    {
        return std::max(lo, std::min(x, hi));
    }

    uint64_t generateId()
    {
        static std::atomic<uint64_t> nextId{ 0 };
        return nextId.fetch_add(1, std::memory_order::memory_order_relaxed);
    }

    template <typename PixelCvt_>
    void ñvtBitmap(FIBITMAP* dst, FIBITMAP* src, PixelCvt_ && pixelConverter)
    {
        const unsigned h = FreeImage_GetHeight(src);
        const unsigned w = FreeImage_GetWidth(src);
        assert(h == FreeImage_GetHeight(dst));
        assert(w == FreeImage_GetWidth(dst));
        const auto srcPixelStride = FreeImage_GetBPP(src) / 8;
        const auto dstPixelStride = FreeImage_GetBPP(dst) / 8;
        for (unsigned j = 0; j < h; ++j) {
            auto dstIt = FreeImage_GetScanLine(dst, j);
            auto srcIt = FreeImage_GetScanLine(src, j);
            for(unsigned i = 0; i < w; ++i, dstIt += dstPixelStride, srcIt += srcPixelStride) {
                pixelConverter(dstIt, srcIt);
            }
        }
    }

    FIBITMAP* FreeImageEx_ToneMapping(FIBITMAP* src, ToneMapping mode)
    {
        if (!src) {
            return nullptr;
        }
        FIBITMAP* dst = nullptr;
        switch(mode) {
            case ToneMapping::FITMO_GLOBAL: {
                    const unsigned h = FreeImage_GetHeight(src);
                    const unsigned w = FreeImage_GetWidth(src);
                    float minVal = std::numeric_limits<float>::max();
                    float maxVal = std::numeric_limits<float>::min();
                    const uint32_t lineLength = w * FreeImage_GetBPP(src) / 4;
                    for (unsigned j = 0; j < h; ++j) {
                        const auto srcLine = static_cast<const float*>(static_cast<const void*>(FreeImage_GetScanLine(src, j)));
                        for (unsigned i = 0; i < lineLength; ++i) {
                            minVal = std::min(minVal, srcLine[i]);
                            maxVal = std::max(maxVal, srcLine[i]);
                        }
                    }
                    if(minVal >= 0.0f && maxVal <= 1.0f) {
                        goto ToneMappingNone;
                    }
                    switch (FreeImage_GetImageType(src)) {
                        case FIT_RGBAF: {
                            dst = FreeImage_Allocate(w, h, 32);
                            ñvtBitmap(dst, src, [&](void* dstPtr, const void* srcPtr) {
                                const auto dstPixel = static_cast<tagRGBQUAD*>(dstPtr);
                                const auto srcPixel = static_cast<const tagFIRGBAF*>(srcPtr);
                                dstPixel->rgbRed      = static_cast<BYTE>(((srcPixel->red   - minVal) / (maxVal - minVal)) * 255.0f);
                                dstPixel->rgbGreen    = static_cast<BYTE>(((srcPixel->green - minVal) / (maxVal - minVal)) * 255.0f);
                                dstPixel->rgbBlue     = static_cast<BYTE>(((srcPixel->blue  - minVal) / (maxVal - minVal)) * 255.0f);
                                dstPixel->rgbReserved = static_cast<BYTE>(((srcPixel->alpha - minVal) / (maxVal - minVal)) * 255.0f);
                            });
                            break;
                        }
                        case FIT_RGBF : {
                            dst = FreeImage_Allocate(w, h, 24);
                            ñvtBitmap(dst, src, [&](void* dstPtr, const void* srcPtr) {
                                const auto dstPixel = static_cast<tagRGBTRIPLE*>(dstPtr);
                                const auto srcPixel = static_cast<const tagFIRGBF*>(srcPtr);
                                dstPixel->rgbtRed   = static_cast<BYTE>(((srcPixel->red   - minVal) / (maxVal - minVal)) * 255.0f);
                                dstPixel->rgbtGreen = static_cast<BYTE>(((srcPixel->green - minVal) / (maxVal - minVal)) * 255.0f);
                                dstPixel->rgbtBlue  = static_cast<BYTE>(((srcPixel->blue  - minVal) / (maxVal - minVal)) * 255.0f);
                            });
                            break;
                        }
                        default:
                            break;
                    }
                }
                break;
            case ToneMapping::FITMO_NONE:
            ToneMappingNone: {
                    const unsigned h = FreeImage_GetHeight(src);
                    const unsigned w = FreeImage_GetWidth(src);
                    switch (FreeImage_GetImageType(src)) {
                        case FIT_RGBAF: {
                            dst = FreeImage_Allocate(w, h, 32);
                            ñvtBitmap(dst, src, [](void* dstPtr, const void* srcPtr) {
                                const auto dstPixel = static_cast<tagRGBQUAD*>(dstPtr);
                                const auto srcPixel = static_cast<const tagFIRGBAF*>(srcPtr);
                                dstPixel->rgbRed      = static_cast<BYTE>(clamp(srcPixel->red)   * 255.0f);
                                dstPixel->rgbGreen    = static_cast<BYTE>(clamp(srcPixel->green) * 255.0f);
                                dstPixel->rgbBlue     = static_cast<BYTE>(clamp(srcPixel->blue)  * 255.0f);
                                dstPixel->rgbReserved = static_cast<BYTE>(clamp(srcPixel->alpha) * 255.0f);
                            });
                            break;
                        }
                        case FIT_RGBF: {
                            dst = FreeImage_Allocate(w, h, 24);
                            ñvtBitmap(dst, src, [](void* dstPtr, const void* srcPtr) {
                                const auto dstPixel = static_cast<tagRGBTRIPLE*>(dstPtr);
                                const auto srcPixel = static_cast<const tagFIRGBF*>(srcPtr);
                                dstPixel->rgbtRed   = static_cast<BYTE>(clamp(srcPixel->red)   * 255.0f);
                                dstPixel->rgbtGreen = static_cast<BYTE>(clamp(srcPixel->green) * 255.0f);
                                dstPixel->rgbtBlue  = static_cast<BYTE>(clamp(srcPixel->blue)  * 255.0f);
                            });
                            break;
                        }
                    default:
                        break;
                    }
                }
                break;
            default:
                dst = FreeImage_ToneMapping(src, static_cast<FREE_IMAGE_TMO>(mode));
                break;
        }
        return dst;
    }

    QImage makeQImageView(FIBITMAP* bmp)
    {
        assert(bmp != nullptr);
        QImage imageView;
        switch (FreeImage_GetBPP(bmp)) {
        case 1:
            imageView = QImage(FreeImage_GetBits(bmp), FreeImage_GetWidth(bmp), FreeImage_GetHeight(bmp), FreeImage_GetPitch(bmp), QImage::Format_Mono);
            break;
        case 8:
            imageView = QImage(FreeImage_GetBits(bmp), FreeImage_GetWidth(bmp), FreeImage_GetHeight(bmp), FreeImage_GetPitch(bmp), QImage::Format_Grayscale8);
            break;
        case 24:
            imageView = QImage(FreeImage_GetBits(bmp), FreeImage_GetWidth(bmp), FreeImage_GetHeight(bmp), FreeImage_GetPitch(bmp), QImage::Format_RGB888);
            break;
        case 32:
            imageView = QImage(FreeImage_GetBits(bmp), FreeImage_GetWidth(bmp), FreeImage_GetHeight(bmp), FreeImage_GetPitch(bmp), QImage::Format_RGBA8888);
            break;
        default:
            throw std::logic_error("Internal image is 1, 8, 24 or 32 bit");
        }
        return imageView;
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
        srcFormat = "RGB float";
        goto ToneMapping;
    case FIT_RGBF:
        srcFormat = "RGB float";
    ToneMapping:
        mIsHDR = true;
        dst = FreeImageEx_ToneMapping(src, mToneMappingMode);
        if (mToneMappingMode != ToneMapping::FITMO_NONE) {
            srcFormat += " (tonemapped)";
        }
        dstNeedUnload = true;
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
    mInvalidTransform = true;
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
    if (mRotation == Rotation::eDegree90 || mRotation == Rotation::eDegree270) {
        return sourceHeight();
    }
    else {
        return sourceWidth();
    }
}

uint32_t Image::height() const
{
    if (mRotation == Rotation::eDegree90 || mRotation == Rotation::eDegree270) {
        return sourceWidth();
    }
    else {
        return sourceHeight();
    }
}

uint32_t Image::sourceWidth() const
{
    return !isNull() ? FreeImage_GetWidth(mBitmapInternal) : 0;
}

uint32_t Image::sourceHeight() const
{
    return !isNull() ? FreeImage_GetHeight(mBitmapInternal) : 0;
}

const QPixmap & Image::pixmap(PageInfo* info) const
{
    if (!isNull()) {
        if (mInvalidPixmap) {
            QString ignoreFormat;
            mInvalidPixmap = !const_cast<Image*>(this)->readCurrentPage(ignoreFormat);
            mInvalidTransform = true;
        }
        if (!mInvalidPixmap && mInvalidTransform) {
            mPixmap = recalculatePixmap();
            mInvalidTransform = false;
        }
    }
    if (info) {
        *info = mPageInfo;
    }
    return mPixmap;
}

QPixmap Image::recalculatePixmap() const
{
    assert(mBitmapInternal != nullptr);

    std::unique_ptr<FIBITMAP, decltype(&::FreeImage_Unload)> rotated(nullptr, &::FreeImage_Unload);
    FIBITMAP* target = mBitmapInternal;
    if (mRotation != Rotation::eDegree0) {
        rotated.reset(FreeImage_Rotate(target, static_cast<double>(toDegree(mRotation))));
        target = rotated.get();
    }

    assert(target != nullptr);

    return QPixmap::fromImage(makeQImageView(target));
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
    }
}

void Image::setToneMapping(ToneMapping mode)
{
    if (mIsHDR) {
        if(mode != mToneMappingMode) {
            mToneMappingMode = mode;
            mInvalidPixmap = true;
        }
    }
}
