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
#include <array>
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

    template <typename DstType_, typename SrcType_, typename PixelCvt_>
    void cvtBitmap(FIBITMAP* dst, FIBITMAP* src, PixelCvt_ pixelConverter)
    {
        const unsigned h = FreeImage_GetHeight(src);
        const unsigned w = FreeImage_GetWidth(src);
        assert(h == FreeImage_GetHeight(dst));
        assert(w == FreeImage_GetWidth(dst));
        for (unsigned j = 0; j < h; ++j) {
            auto dstLine = static_cast<DstType_*>(static_cast<void*>(FreeImage_GetScanLine(dst, j)));
            auto srcLine = static_cast<const SrcType_*>(static_cast<const void*>(FreeImage_GetScanLine(src, j)));
            for(unsigned i = 0; i < w; ++i, ++dstLine, ++srcLine) {
                pixelConverter(dstLine, srcLine);
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
                cvtBitmap<tagRGBQUAD, tagFIRGBAF>(dst, src, [](tagRGBQUAD* dstPixel, const tagFIRGBAF* srcPixel) {
                    dstPixel->rgbRed      = static_cast<BYTE>(clamp(srcPixel->red)   * 255.0f);
                    dstPixel->rgbGreen    = static_cast<BYTE>(clamp(srcPixel->green) * 255.0f);
                    dstPixel->rgbBlue     = static_cast<BYTE>(clamp(srcPixel->blue)  * 255.0f);
                    dstPixel->rgbReserved = static_cast<BYTE>(clamp(srcPixel->alpha) * 255.0f);
                });
                break;
            }
            case FIT_RGBF: {
                dst = FreeImage_Allocate(w, h, 24);
                cvtBitmap<tagRGBTRIPLE, tagFIRGBF>(dst, src, [](tagRGBTRIPLE* dstPixel, const tagFIRGBF* srcPixel) {
                    dstPixel->rgbtRed   = static_cast<BYTE>(clamp(srcPixel->red)   * 255.0f);
                    dstPixel->rgbtGreen = static_cast<BYTE>(clamp(srcPixel->green) * 255.0f);
                    dstPixel->rgbtBlue  = static_cast<BYTE>(clamp(srcPixel->blue)  * 255.0f);
                });
                break;
            }
            case FIT_DOUBLE: {
                dst = FreeImage_Allocate(w, h, 8);
                cvtBitmap<BYTE, double>(dst, src, [](BYTE* dstPixel, const double* srcPixel) {
                    *dstPixel = static_cast<BYTE>(clamp(*srcPixel) * 255.0);
                });
                break;
            }
            case FIT_FLOAT: {
                dst = FreeImage_Allocate(w, h, 8);
                cvtBitmap<BYTE, float>(dst, src, [](BYTE* dstPixel, const float* srcPixel) {
                    *dstPixel = static_cast<BYTE>(clamp(*srcPixel) * 255.0f);
                });
                break;
            }
            default:
                break;
        }
        return dst;
    }


    template <typename ElemType_, typename Less_ = std::less<>>
    std::tuple<ElemType_*, ElemType_*> findMinMax(FIBITMAP* src, Less_ less = Less_{})
    {
        if (!src) {
            return std::tuple<ElemType_*, ElemType_*>(nullptr, nullptr);
        }
        const unsigned h = FreeImage_GetHeight(src);
        const unsigned w = FreeImage_GetWidth(src);
        if (h < 1 || w < 1) {
            return std::tuple<ElemType_*, ElemType_*>(nullptr, nullptr);
        }
        ElemType_* minIt = static_cast<ElemType_*>(static_cast<void*>(FreeImage_GetScanLine(src, 0)));
        ElemType_* maxIt = minIt;
        for (unsigned j = 0; j < h; ++j) {
            auto srcLine = static_cast<ElemType_*>(static_cast<void*>(FreeImage_GetScanLine(src, j)));
            for (unsigned i = 0; i < w; ++i, ++srcLine) {
                if (less(*srcLine, *minIt)) {
                    minIt = srcLine;
                }
                if (less(*maxIt, *srcLine)) {
                    maxIt = srcLine;
                }
            }
        }
        return std::make_tuple(minIt, maxIt);
    }

    template <typename Ty_>
    inline
    Ty_ GetBrightness(Ty_ r, Ty_ g, Ty_ b)
    {
        return static_cast<Ty_>(0.114 * b + 0.587 * g + 0.299 * r);
    }

    inline
    float GetBrightness(const tagFIRGBF& p)
    {
        return GetBrightness(p.red, p.green, p.blue);
    }

    inline
    float GetBrightness(const tagFIRGBAF& p)
    {
        return GetBrightness(p.red, p.green, p.blue);
    }

    inline
    bool LessBrightness(const tagRGBTRIPLE& p1, const tagRGBTRIPLE& p2)
    {
        const uint32_t b1 = 114 * p1.rgbtBlue + 587 * p1.rgbtGreen + 299 * p1.rgbtRed;
        const uint32_t b2 = 114 * p2.rgbtBlue + 587 * p2.rgbtGreen + 299 * p2.rgbtRed;
        return b1 < b2;
    }

    inline
    bool LessBrightness(const tagFIRGBF& p1, const tagFIRGBF& p2)
    {
        return GetBrightness(p1) < GetBrightness(p2);
    }

    inline
    bool LessBrightness(const tagFIRGBAF& p1, const tagFIRGBAF& p2)
    {
        return GetBrightness(p1) < GetBrightness(p2);
    }

    template <typename Ty_>
    inline
    std::array<Ty_, 3> RgbToYuv(Ty_ r, Ty_ g, Ty_ b)
    {
        return {
            static_cast<Ty_>(0.114 * b + 0.587 * g + 0.299 * r),
            static_cast<Ty_>(0.5 * b - 0.3313 * g - 0.1687 * r),
            static_cast<Ty_>(0.5 * r - 0.4187 * g - 0.0813 * b)
        };
    }

    inline
    std::array<float, 3> RgbToYuv(const std::array<float, 3>& p)
    {
        return RgbToYuv(p[0], p[1], p[2]);
    }


    template <typename Ty_>
    inline
    std::array<Ty_, 3> YuvToRgb(Ty_ y, Ty_ cb, Ty_ cr)
    {
        return {
            static_cast<Ty_>(y + 1.402 * cr),
            static_cast<Ty_>(y - 0.3441 * cb - 0.7141 * cr),
            static_cast<Ty_>(y + 1.772 * cb)
        };
    }

    inline
    std::array<float, 3> YuvToRgb(const std::array<float, 3>& p)
    {
        return YuvToRgb(p[0], p[1], p[2]);
    }


    FIBITMAP* applyToneMappingGlobal(FIBITMAP* src)
    {
        assert(src);
        FIBITMAP* dst = nullptr;
        const unsigned h = FreeImage_GetHeight(src);
        const unsigned w = FreeImage_GetWidth(src);
        switch (FreeImage_GetImageType(src)) {
            case FIT_RGBAF: {
                tagFIRGBAF* minVal = nullptr;
                tagFIRGBAF* maxVal = nullptr;
                std::tie(minVal, maxVal) = findMinMax<tagFIRGBAF>(src, static_cast<bool(*)(const tagFIRGBAF&, const tagFIRGBAF&)>(&LessBrightness));
                if (minVal != maxVal) {
                    const float maxBrighness = GetBrightness(*maxVal);
                    const float minBrighness = GetBrightness(*minVal);
                    if (maxBrighness > 0 && maxBrighness > minBrighness) {
                        const float black = minBrighness / maxBrighness;
                        const float div = 1.0f / (1.0f - black);
                        dst = FreeImage_Allocate(w, h, 32);
                        cvtBitmap<tagRGBQUAD, tagFIRGBAF>(dst, src, [&](tagRGBQUAD* dstPixel, const tagFIRGBAF* srcPixel) {
                            auto yuv = RgbToYuv(srcPixel->red / maxBrighness, srcPixel->green / maxBrighness, srcPixel->blue / maxBrighness);
                            yuv[0] = (yuv[0] - black) * div;
                            auto rgb = YuvToRgb(yuv);
                            dstPixel->rgbRed      = static_cast<BYTE>(std::clamp(rgb[0], 0.0f, 1.0f) * 255.0f);
                            dstPixel->rgbGreen    = static_cast<BYTE>(std::clamp(rgb[1], 0.0f, 1.0f) * 255.0f);
                            dstPixel->rgbBlue     = static_cast<BYTE>(std::clamp(rgb[2], 0.0f, 1.0f) * 255.0f);
                            dstPixel->rgbReserved = static_cast<BYTE>(std::clamp(srcPixel->alpha, 0.0f, 1.0f) * 255.0f);
                        });
                    }
                }
                break;
            }
            case FIT_RGBF : {
                tagFIRGBF* minVal = nullptr;
                tagFIRGBF* maxVal = nullptr;
                std::tie(minVal, maxVal) = findMinMax<tagFIRGBF>(src, static_cast<bool(*)(const tagFIRGBF&, const tagFIRGBF&)>(&LessBrightness));
                if (minVal != maxVal) {
                    const float maxBrighness = GetBrightness(*maxVal);
                    const float minBrighness = GetBrightness(*minVal);
                    if (maxBrighness > 0 && maxBrighness > minBrighness) {
                        const float black = minBrighness / maxBrighness;
                        const float div = 1.0f / (1.0f - black);
                        dst = FreeImage_Allocate(w, h, 24);
                        cvtBitmap<tagRGBTRIPLE, tagFIRGBF>(dst, src, [&](tagRGBTRIPLE* dstPixel, const tagFIRGBF* srcPixel) {
                            auto yuv = RgbToYuv(srcPixel->red / maxBrighness, srcPixel->green / maxBrighness, srcPixel->blue / maxBrighness);
                            yuv[0] = (yuv[0] - black) * div;
                            auto rgb = YuvToRgb(yuv);
                            dstPixel->rgbtRed   = static_cast<BYTE>(std::clamp(rgb[0], 0.0f, 1.0f) * 255.0f);
                            dstPixel->rgbtGreen = static_cast<BYTE>(std::clamp(rgb[1], 0.0f, 1.0f) * 255.0f);
                            dstPixel->rgbtBlue  = static_cast<BYTE>(std::clamp(rgb[2], 0.0f, 1.0f) * 255.0f);
                        });
                    }
                }
                break;
            }
            case FIT_DOUBLE: {
                double* minVal = nullptr;
                double* maxVal = nullptr;
                std::tie(minVal, maxVal) = findMinMax<double>(src);
                if ((minVal != maxVal) && (*maxVal > *minVal)) {
                    double div = 255.0f / (*maxVal - *minVal);
                    dst = FreeImage_Allocate(w, h, 8);
                    cvtBitmap<BYTE, double>(dst, src, [&](BYTE* dstPixel, const double* srcPixel) {
                        *dstPixel = static_cast<BYTE>((*srcPixel - *minVal) * div);
                    });
                }
                break;
            }
            case FIT_FLOAT: {
                float* minVal = nullptr;
                float* maxVal = nullptr;
                std::tie(minVal, maxVal) = findMinMax<float>(src);
                if ((minVal != maxVal) && (*maxVal > *minVal)) {
                    float div = 255.0f / (*maxVal - *minVal);
                    dst = FreeImage_Allocate(w, h, 8);
                    cvtBitmap<BYTE, float>(dst, src, [&](BYTE* dstPixel, const float* srcPixel) {
                        *dstPixel = static_cast<BYTE>((*srcPixel - *minVal) * div);
                    });
                }
                break;
            }
            default:
                break;
        }
        if (!dst) {
            // If failed try to at least convert
            dst = applyToneMappingNone(src);
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
        case 64:
        case 128:
            return 4;
        case 24:
        case 48:
        case 96:
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


FIBITMAP* FreeImageExt_ConvertToFloat(FIBITMAP* src)
{
    if (!src) {
        return nullptr;
    }
    const unsigned h = FreeImage_GetHeight(src);
    const unsigned w = FreeImage_GetWidth(src);
    if (!h || !w) {
        return nullptr;
    }
    FIBITMAP* dst = nullptr;
    switch(FreeImage_GetImageType(src)) {
    case FIT_INT16:
        dst = FreeImage_AllocateT(FIT_FLOAT, w, h, 32);
        cvtBitmap<float, int16_t>(dst, src, [&](float* dstPtr, const int16_t* srcPtr) {
            *dstPtr = static_cast<float>(*srcPtr);
        });
        break;
    case FIT_UINT16:
        dst = FreeImage_AllocateT(FIT_FLOAT, w, h, 32);
        cvtBitmap<float, uint16_t>(dst, src, [&](float* dstPtr, const uint16_t* srcPtr) {
            *dstPtr = static_cast<float>(*srcPtr);
        });
        break;
    case FIT_INT32:
        dst = FreeImage_AllocateT(FIT_FLOAT, w, h, 32);
        cvtBitmap<float, int32_t>(dst, src, [&](float* dstPtr, const int32_t* srcPtr) {
            *dstPtr = static_cast<float>(*srcPtr);
        });
        break;
    case FIT_UINT32:
        dst = FreeImage_AllocateT(FIT_FLOAT, w, h, 32);
        cvtBitmap<float, uint32_t>(dst, src, [&](float* dstPtr, const uint32_t* srcPtr) {
            *dstPtr = static_cast<float>(*srcPtr);
        });
        break;
    default:
        dst = FreeImage_ConvertToFloat(src);
        break;
    }
    return dst;
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


