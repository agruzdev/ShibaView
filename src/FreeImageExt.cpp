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

#include "FreeImageExt.h"

#include <algorithm>
#include <cassert>
#include <tuple>
#include "PluginFLO.h"
#include "PluginSVG.h"

namespace
{
    template <typename Ty_>
    const Ty_ & clamp(const Ty_ & x, const Ty_ & lo = static_cast<Ty_>(0), const Ty_ & hi = static_cast<Ty_>(1))
    {
        return std::max(lo, std::min(x, hi));
    }

    template <typename PixelCvt_>
    void cvtBitmap(FIBITMAP* dst, FIBITMAP* src, PixelCvt_ && pixelConverter)
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

    FIBITMAP* applyToneMappingNone(FIBITMAP* src)
    {
        assert(src);
        FIBITMAP* dst = nullptr;
        const unsigned h = FreeImage_GetHeight(src);
        const unsigned w = FreeImage_GetWidth(src);
        switch (FreeImage_GetImageType(src)) {
            case FIT_RGBAF: {
                dst = FreeImage_Allocate(w, h, 32);
                cvtBitmap(dst, src, [](void* dstPtr, const void* srcPtr) {
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
                cvtBitmap(dst, src, [](void* dstPtr, const void* srcPtr) {
                    const auto dstPixel = static_cast<tagRGBTRIPLE*>(dstPtr);
                    const auto srcPixel = static_cast<const tagFIRGBF*>(srcPtr);
                    dstPixel->rgbtRed   = static_cast<BYTE>(clamp(srcPixel->red)   * 255.0f);
                    dstPixel->rgbtGreen = static_cast<BYTE>(clamp(srcPixel->green) * 255.0f);
                    dstPixel->rgbtBlue  = static_cast<BYTE>(clamp(srcPixel->blue)  * 255.0f);
                });
                break;
            }
            case FIT_DOUBLE: {
                dst = FreeImage_Allocate(w, h, 8);
                cvtBitmap(dst, src, [](void* dstPtr, const void* srcPtr) {
                    const auto dstPixel = static_cast<BYTE*>(dstPtr);
                    const auto srcPixel = static_cast<const double*>(srcPtr);
                    *dstPixel = static_cast<BYTE>(clamp(*srcPixel) * 255.0);
                });
                break;
            }
            case FIT_FLOAT: {
                dst = FreeImage_Allocate(w, h, 8);
                cvtBitmap(dst, src, [](void* dstPtr, const void* srcPtr) {
                    const auto dstPixel = static_cast<BYTE*>(dstPtr);
                    const auto srcPixel = static_cast<const float*>(srcPtr);
                    *dstPixel = static_cast<BYTE>(clamp(*srcPixel) * 255.0f);
                });
                break;
            }
            default:
                break;
        }
        return dst;
    }

    template <typename Ty_>
    std::tuple<Ty_, Ty_> findMinMax(FIBITMAP* src)
    {
        assert(src);
        const unsigned h = FreeImage_GetHeight(src);
        const unsigned w = FreeImage_GetWidth(src);
        Ty_ minVal = std::numeric_limits<Ty_>::max();
        Ty_ maxVal = std::numeric_limits<Ty_>::min();
        const uint32_t lineLength = w * (FreeImage_GetBPP(src) / 8 / sizeof(Ty_));
        for (unsigned j = 0; j < h; ++j) {
            const auto srcLine = static_cast<const Ty_*>(static_cast<const void*>(FreeImage_GetScanLine(src, j)));
            for (unsigned i = 0; i < lineLength; ++i) {
                minVal = std::min(minVal, srcLine[i]);
                maxVal = std::max(maxVal, srcLine[i]);
            }
        }
        return std::make_tuple(minVal, maxVal);
    }

    FIBITMAP* applyToneMappingGlobal(FIBITMAP* src)
    {
        assert(src);
        FIBITMAP* dst = nullptr;
        const unsigned h = FreeImage_GetHeight(src);
        const unsigned w = FreeImage_GetWidth(src);
        switch (FreeImage_GetImageType(src)) {
            case FIT_RGBAF: {
                float minVal = 0.0f, maxVal = 1.0f;
                std::tie(minVal, maxVal) = findMinMax<float>(src);
                dst = FreeImage_Allocate(w, h, 32);
                cvtBitmap(dst, src, [&](void* dstPtr, const void* srcPtr) {
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
                float minVal = 0.0f, maxVal = 1.0f;
                std::tie(minVal, maxVal) = findMinMax<float>(src);
                dst = FreeImage_Allocate(w, h, 24);
                cvtBitmap(dst, src, [&](void* dstPtr, const void* srcPtr) {
                    const auto dstPixel = static_cast<tagRGBTRIPLE*>(dstPtr);
                    const auto srcPixel = static_cast<const tagFIRGBF*>(srcPtr);
                    dstPixel->rgbtRed   = static_cast<BYTE>(((srcPixel->red   - minVal) / (maxVal - minVal)) * 255.0f);
                    dstPixel->rgbtGreen = static_cast<BYTE>(((srcPixel->green - minVal) / (maxVal - minVal)) * 255.0f);
                    dstPixel->rgbtBlue  = static_cast<BYTE>(((srcPixel->blue  - minVal) / (maxVal - minVal)) * 255.0f);
                });
                break;
            }
            case FIT_DOUBLE: {
                double minVal = 0.0f, maxVal = 1.0f;
                std::tie(minVal, maxVal) = findMinMax<double>(src);
                dst = FreeImage_Allocate(w, h, 8);
                cvtBitmap(dst, src, [&](void* dstPtr, const void* srcPtr) {
                    const auto dstPixel = static_cast<BYTE*>(dstPtr);
                    const auto srcPixel = static_cast<const double*>(srcPtr);
                    *dstPixel = static_cast<BYTE>(((*srcPixel - minVal) / (maxVal - minVal)) * 255.0f);
                });
                break;
            }
            case FIT_FLOAT: {
                float minVal = 0.0f, maxVal = 1.0f;
                std::tie(minVal, maxVal) = findMinMax<float>(src);
                dst = FreeImage_Allocate(w, h, 8);
                cvtBitmap(dst, src, [&](void* dstPtr, const void* srcPtr) {
                    const auto dstPixel = static_cast<BYTE*>(dstPtr);
                    const auto srcPixel = static_cast<const float*>(srcPtr);
                    *dstPixel = static_cast<BYTE>(((*srcPixel - minVal) / (maxVal - minVal)) * 255.0f);
                });
                break;
            }
            default:
                break;
        }
        return dst;
    }
}


void FreeImageExt_Initialise()
{
    FreeImage_RegisterLocalPlugin(&initPluginFLO);
    FreeImage_RegisterLocalPlugin(&initPluginSVG);
}


void FreeImageExt_DeInitialise()
{
}


DWORD FreeImageExt_GetChannelsNumber(FIBITMAP* dib)
{
    if (!dib) {
        return 0;
    }

    switch(FreeImage_GetImageType(dib)) {
    case FIT_BITMAP:
        switch(FreeImage_GetBPP(dib)) {
        case 32:
            return 4;
        case 24:
            return 3;
        default:
            return 1;
        }

    case FIT_RGB16:
    case FIT_RGBF:
        return 3;

    case FIT_RGBA16:
    case FIT_RGBAF:
        return 4;

    default:
        return 1;
    }
}


FIBITMAP* FreeImageExt_ToneMapping(FIBITMAP* src, FIE_ToneMapping mode)
{
    FIBITMAP* dst = nullptr;
    if (src) {
        switch(mode) {
            case FIETMO_NONE:
                dst = applyToneMappingNone(src);
                break;
            case FIETMO_LINEAR:
                dst = applyToneMappingGlobal(src);
                break;
            default: {
                //const auto fit = FreeImage_GetImageType(src);
                //if (fit == FIT_RGBF || fit == FIT_RGBAF) {
                    dst = FreeImage_ToneMapping(src, static_cast<FREE_IMAGE_TMO>(mode));
                //}
                break;
            }
        }
    }
    return dst;
}


const char* FreeImageExt_TMtoString(FIE_ToneMapping mode)
{
    switch(mode) {
        case FIETMO_NONE:
            return "None";
        case FIETMO_LINEAR:
            return "Linear";
        case FIETMO_DRAGO03:
            return "F.Drago, 2003";
        case FIETMO_REINHARD05:
            return "E. Reinhard, 2005";
        case FIETMO_FATTAL02:
            return "R. Fattal, 2002";
        default:
            return nullptr;
    }
}


BOOL FreeImageExt_Draw(FIBITMAP* dst, FIBITMAP* src, FIE_AlphaFunction alpha, int left, int top)
{
    if (!dst || !src || FreeImage_GetImageType(dst) != FIT_BITMAP || FreeImage_GetBPP(dst) != 32 ||
            FreeImage_GetImageType(src) != FIT_BITMAP || FreeImage_GetBPP(src) != 32) {
        return FALSE;
    }

    const int32_t dstW = static_cast<int32_t>(FreeImage_GetWidth(dst));
    const int32_t dstH = static_cast<int32_t>(FreeImage_GetHeight(dst));
    const int32_t srcW = static_cast<int32_t>(FreeImage_GetWidth(src));
    const int32_t srcH = static_cast<int32_t>(FreeImage_GetHeight(src));

    if (left + srcW <= 0 || top + srcH <= 0) {
        return TRUE;
    }

    const int32_t roiLeft   = std::max(0, left);
    const int32_t roiTop    = std::max(0, top);
    const int32_t roiRight  = std::min(left + srcW, dstW);
    const int32_t roiBottom = std::min(top  + srcH, dstH);

    const int32_t offsetX = roiLeft - left;
    const int32_t offsetY = roiTop  - top;

    if (alpha != FIEAF_SrcAlpha) {
        // not implemented
        return FALSE;
    }

    // Y axis is flipped in FI
    for (int32_t y = roiBottom - roiTop; y > 0; --y) {
        const auto srcLine = static_cast<const RGBQUAD*>(static_cast<const void*>(FreeImage_GetScanLine(src, srcH - y - offsetY))) + offsetX;
        const auto dstLine = static_cast<RGBQUAD*>(static_cast<void*>(FreeImage_GetScanLine(dst, dstH - roiTop - y))) + roiLeft;
        for (int32_t x = 0; x < roiRight - roiLeft; ++x) {
            const BYTE A = srcLine[x].rgbReserved;
            if (A == 255) {
                dstLine[x] = srcLine[x];
            }
            else if(A > 0) {
                const BYTE nA = ~A;
                dstLine[x].rgbRed   = static_cast<BYTE>((A * srcLine[x].rgbRed   + nA * dstLine[x].rgbRed)   / 255);
                dstLine[x].rgbGreen = static_cast<BYTE>((A * srcLine[x].rgbGreen + nA * dstLine[x].rgbGreen) / 255);
                dstLine[x].rgbBlue  = static_cast<BYTE>((A * srcLine[x].rgbBlue  + nA * dstLine[x].rgbBlue)  / 255);
                dstLine[x].rgbReserved = A;
            }
        }
    }

    return TRUE;
}

const char* FreeImageExt_DescribeImageType(FIBITMAP* dib)
{
    if (dib) {
        switch (FreeImage_GetImageType(dib)) {
        case FIT_RGBAF:
            return "RGBA Float32";

        case FIT_RGBF:
            return "RGB Float32";

        case FIT_RGBA16:
            return "RGBA16";

        case FIT_RGB16:
            return "RGB16";

        case FIT_UINT16:
            return "Greyscale 16bit";

        case FIT_INT16:
            return "Greyscale 16bit (signed)";

        case FIT_UINT32:
            return "Greyscale 32bit";

        case FIT_INT32:
            return "Greyscale 32bit (signed)";

        case FIT_FLOAT:
            return "Greyscale Float32";

        case FIT_DOUBLE:
            return "Greyscale Float64";

        case FIT_BITMAP:
            switch(FreeImage_GetBPP(dib)) {

            case 32:
                return "RGBA8888";

            case 24:
                return "RGB888";

            case 8:
                if (FIC_PALETTE == FreeImage_GetColorType(dib)) {
                    return "RGB Indexed 8bit";
                }
                else {
                    return "Greyscale 8bit";
                }

            case 4:
                return "RGB Indexed 4bit";

            case 1:
                 if (FIC_PALETTE == FreeImage_GetColorType(dib)) {
                     return "RGB Indexed 1bit";
                 }
                 else {
                     return "Binary image";
                 }

            default:
                break;
            }
            break;

        default:
            break;
        }
    }
    return "Unknown";
}


