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

#include "ImagePage.h"
#include <cassert>
#include <stdexcept>
#include "PluginFLO.h"

namespace
{

    ImageFrame cvtToInternalType(FIBITMAP* src, bool& dstNeedUnload)
    {
        assert(src != nullptr);
        ImageFrame frame{};
        const uint32_t bpp = FreeImage_GetBPP(src);
        switch (FreeImage_GetImageType(src)) {
        case FIT_RGBAF:
            frame.flags = FrameFlags::eHRD | FrameFlags::eRGB;
            frame.bmp = src;
            dstNeedUnload = false;
            break;

        case FIT_RGBF:
            frame.flags = FrameFlags::eHRD | FrameFlags::eRGB;
            frame.bmp = src;
            dstNeedUnload = false;
            break;

        case FIT_RGBA16:
        case FIT_RGBA32:
            frame.flags = FrameFlags::eHRD | FrameFlags::eRGB;
            frame.bmp = FreeImage_ConvertToRGBAF(src);
            dstNeedUnload = true;
            break;

        case FIT_RGB16:
        case FIT_RGB32:
            frame.flags = FrameFlags::eHRD | FrameFlags::eRGB;
            frame.bmp = FreeImage_ConvertToRGBF(src);
            dstNeedUnload = true;
            break;

        case FIT_UINT16:
        case FIT_INT16:
        case FIT_UINT32:
        case FIT_INT32:
            frame.bmp = FreeImage_ConvertToFloat(src);
            frame.flags = FrameFlags::eHRD;
            dstNeedUnload = true;
            break;

        case FIT_FLOAT:
            frame.flags = FrameFlags::eHRD;
            frame.bmp = src;
            dstNeedUnload = false;
            break;

        case FIT_DOUBLE:
            frame.flags = FrameFlags::eHRD;
            frame.bmp = src;
            dstNeedUnload = false;
            break;

        case FIT_COMPLEXF:
        case FIT_COMPLEX:
            frame.flags = FrameFlags::eNone;
            frame.bmp = cvtFloToRgb(src);
            dstNeedUnload = true;
            break;

        case FIT_BITMAP:
            if (32 == bpp) {
                frame.flags = FrameFlags::eRGB;
                frame.bmp = src;
                dstNeedUnload = false;
            }
            else if (24 == bpp) {
                frame.flags = FrameFlags::eRGB;
                frame.bmp = src;
                dstNeedUnload = false;
            }
            else if (8 == bpp) {
                const auto colorType = FreeImage_GetColorType(src);
                if (FIC_PALETTE == colorType) {
                    //FreeImage_Save(FIF_TIFF, src, "test.tiff");

                    frame.flags = FrameFlags::eRGB;
                    frame.bmp = FreeImage_ConvertTo32Bits(src);
                    dstNeedUnload = true;
                }
                else if (FIC_MINISWHITE == colorType) {
                    frame.bmp = FreeImage_Clone(src);
                    FreeImage_Invert(frame.bmp);
                    dstNeedUnload = true;
                }
                else {
                    frame.bmp = src;
                    dstNeedUnload = false;
                }
            }
            else if (4 == bpp) {
                frame.bmp = FreeImage_ConvertTo32Bits(src);
                frame.flags = FrameFlags::eRGB;
                dstNeedUnload = true;
            }
            else if (1 == bpp) {
                const auto colorType = FreeImage_GetColorType(src);
                if (FIC_PALETTE == colorType) {
                    frame.flags = FrameFlags::eRGB;
                    frame.bmp = FreeImage_ConvertTo32Bits(src);
                    dstNeedUnload = true;
                }
                else if (FIC_MINISWHITE == colorType) {
                    frame.bmp = FreeImage_Clone(src);
                    FreeImage_Invert(frame.bmp);
                    dstNeedUnload = true;
                }
                else {
                    frame.bmp = src;
                    dstNeedUnload = false;
                }
            }
            break;

        default:
            break;
        }
        return frame;
    }

} // namespace

ImagePage::ImagePage(FIBITMAP* bmp, uint32_t index)
    : mBitmap(bmp)
    , mIndex(index)
{
    if (!mBitmap) {
        throw std::runtime_error("ImagePage[ctor]: Page bitmap is null.");
    }
    mConvertedFrame = cvtToInternalType(mBitmap, mFrameNeedsUnload);
    if (!mConvertedFrame.bmp) {
        throw std::runtime_error("ImagePage[ctor]: Failed to convert a frame.");
    }
}

ImagePage::~ImagePage()
{
    if (mFrameNeedsUnload && mConvertedFrame.bmp) {
        FreeImage_Unload(mConvertedFrame.bmp);
    }
}

QString ImagePage::doDescribeFormat() const
{
    const std::string customDescription = FreeImageExt_GetMetadataValue<std::string>(FIMD_CUSTOM, mBitmap, "ImageType", "");
    if (!customDescription.empty()) {
        return QString::fromUtf8(customDescription);
    }
    return FreeImageExt_DescribeImageType(mBitmap);
}

bool ImagePage::doGetPixel(uint32_t y, uint32_t x, Pixel* pixel) const
{
    return Pixel::getBitmapPixel(mBitmap, y, x, pixel);
}

Exif ImagePage::doGetExif() const
{
    return Exif::load(mBitmap);
}

size_t ImagePage::getMemorySize() const
{
    return FreeImage_GetMemorySize(mBitmap) + FreeImage_GetMemorySize(mConvertedFrame.bmp);
}

UniqueBitmap ImagePage::getOrMakeThumbnail(uint32_t maxSize) const
{
    UniqueBitmap result(nullptr, &FreeImage_Unload);
    if (FIBITMAP* storedThumbnail = FreeImage_GetThumbnail(mBitmap)) {
        const unsigned w = FreeImage_GetWidth(storedThumbnail);
        const unsigned h = FreeImage_GetHeight(storedThumbnail);
        if (w > maxSize || h > maxSize) {
            const unsigned size = std::max(w, h);
            result.reset(FreeImage_Rescale(storedThumbnail, w * maxSize / size, h * maxSize / size, FILTER_BICUBIC));
        }
        else {
            result.reset(FreeImage_Clone(storedThumbnail));
        }
    }
    else if (mConvertedFrame.bmp) {
        FIBITMAP* ldrFrame = mConvertedFrame.bmp;
        if ((mConvertedFrame.flags & FrameFlags::eHRD) != FrameFlags::eNone) {
            ldrFrame = FreeImage_ToneMapping(mConvertedFrame.bmp, FITMO_LINEAR);
        }
        if (ldrFrame) {
            const unsigned w = FreeImage_GetWidth(ldrFrame);
            const unsigned h = FreeImage_GetHeight(ldrFrame);
            const unsigned size = std::max(w, h);
            result.reset(FreeImage_Rescale(ldrFrame, w * maxSize / size, h * maxSize / size, FILTER_BICUBIC));
        }
        if (ldrFrame == mConvertedFrame.bmp) {
            ldrFrame = FreeImage_Clone(mConvertedFrame.bmp);
        }
        result.reset(ldrFrame);
    }
    return result;
}
