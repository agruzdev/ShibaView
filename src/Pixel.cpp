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

#include "Pixel.h"

bool Pixel::getBitmapPixel(FIBITMAP* src, uint32_t y, uint32_t x, Pixel* pixel)
{
    if (!src) {
        return false;
    }
    if (y >= FreeImage_GetHeight(src) || x >= FreeImage_GetWidth(src)) {
        return false;
    }
    bool success = true;
    const uint32_t bpp = FreeImage_GetBPP(src);
    const uint8_t* rawPixel = FreeImage_GetScanLine(src, static_cast<int>(y)) + x * bpp / 8;
    switch (FreeImage_GetImageType(src)) {
    case FIT_RGBAF:
        pixel->repr = pixelToString4<FIRGBAF>(rawPixel);
        break;

    case FIT_RGBF:
        pixel->repr = pixelToString3<FIRGBF>(rawPixel);
        break;

    case FIT_RGBA16:
        pixel->repr = pixelToString4<FIRGBA16>(rawPixel);
        break;

    case FIT_RGB16:
        pixel->repr = pixelToString3<FIRGB16>(rawPixel);
        break;

    case FIT_RGBA32:
        pixel->repr = pixelToString4<FIRGBA32>(rawPixel);
        break;

    case FIT_RGB32:
        pixel->repr = pixelToString3<FIRGB32>(rawPixel);
        break;

    case FIT_UINT16:
        pixel->repr = pixelToString1<uint16_t>(rawPixel);
        break;

    case FIT_INT16:
        pixel->repr = pixelToString1<int16_t>(rawPixel);
        break;

    case FIT_UINT32:
        pixel->repr = pixelToString1<uint32_t>(rawPixel);
        break;

    case FIT_INT32:
        pixel->repr = pixelToString1<int32_t>(rawPixel);
        break;

    case FIT_FLOAT:
        pixel->repr = pixelToString1<float>(rawPixel);
        break;

    case FIT_DOUBLE:
        pixel->repr = pixelToString1<double>(rawPixel);
        break;

    case FIT_BITMAP: {
            if (FIC_PALETTE == FreeImage_GetColorType(src)) {
                const FIRGBA8* palette = FreeImage_GetPalette(src);
                if(!palette) {
                    success = false;
                    break;
                }
                uint8_t index = 0;
                if (!FreeImage_GetPixelIndex(src, x, y, &index)) {
                    success = false;
                    break;
                }
                FIRGBA8 rgba = palette[index];
                if (FreeImage_IsTransparent(src)) {
                    const uint8_t* transparency = FreeImage_GetTransparencyTable(src);
                    const int alphaIndex = FreeImage_GetTransparentIndex(src);
                    if(!transparency || alphaIndex < 0) {
                        success = false;
                        break;
                    }
                    rgba.alpha = transparency[alphaIndex];
                    pixel->repr = pixelToString4<FIRGBA8>(static_cast<const uint8_t*>(static_cast<const void*>(&rgba)));
                }
                else {
                    pixel->repr = pixelToString3<FIRGBA8>(static_cast<const uint8_t*>(static_cast<const void*>(&rgba)));
                }
            }
            else {
                switch(bpp) {
                    case 32:
                        pixel->repr = pixelToString4<FIRGBA8>(rawPixel);
                        break;
                    case 24:
                        pixel->repr = pixelToString3<FIRGB8>(rawPixel);
                        break;
                    case 16:
                        pixel->repr = pixelToString1<uint16_t>(rawPixel);
                        break;
                    case 8:
                        pixel->repr = pixelToString1<uint8_t>(rawPixel);
                        break;
                    default:
                        success = false;
                        break;
                }
            }
            break;
        }
    default:
        success = false;
        break;
    }
    return success;
}

