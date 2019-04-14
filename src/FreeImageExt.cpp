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

#include "FreeImageExt.h"

#include <algorithm>
#include <cassert>

namespace
{
    float clamp(float x, float lo = 0.0f, float hi = 1.0f)
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
            default:
                break;
        }
        return dst;
    }

    FIBITMAP* applyToneMappingGlobal(FIBITMAP* src)
    {
        FIBITMAP* dst = nullptr;
        const unsigned h = FreeImage_GetHeight(src);
        const unsigned w = FreeImage_GetWidth(src);
        float minVal = std::numeric_limits<float>::max();
        float maxVal = std::numeric_limits<float>::min();
        const uint32_t lineLength = w * (FreeImage_GetBPP(src) / 8 / sizeof(float));
        for (unsigned j = 0; j < h; ++j) {
            const auto srcLine = static_cast<const float*>(static_cast<const void*>(FreeImage_GetScanLine(src, j)));
            for (unsigned i = 0; i < lineLength; ++i) {
                minVal = std::min(minVal, srcLine[i]);
                maxVal = std::max(maxVal, srcLine[i]);
            }
        }
        if(minVal >= 0.0f && maxVal <= 1.0f) {
            dst = applyToneMappingNone(src);
        }
        else {
            switch (FreeImage_GetImageType(src)) {
                case FIT_RGBAF: {
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
                default:
                    break;
            }
        }
        return dst;
    }
}

FIBITMAP* FreeImageExt_ToneMapping(FIBITMAP* src, FIE_ToneMapping mode)
{
    FIBITMAP* dst = nullptr;
    if(src) {
        switch(mode) {
            case FIETMO_NONE:
                dst = applyToneMappingNone(src);
                break;
            case FIETMO_LINEAR:
                dst = applyToneMappingGlobal(src);
                break;
            default:
                dst = FreeImage_ToneMapping(src, static_cast<FREE_IMAGE_TMO>(mode));
                break;
        }
    }
    return dst;
}
