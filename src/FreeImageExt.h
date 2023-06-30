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

#ifndef FREEIMAGEEXT_H
#define FREEIMAGEEXT_H

#ifdef __cplusplus
# include <type_traits>
# include <memory>
#endif

#include "FreeImage.h"

typedef struct {
#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR
  BYTE rgbtBlue;
  BYTE rgbtGreen;
  BYTE rgbtRed;
#else
  BYTE rgbtRed;
  BYTE rgbtGreen;
  BYTE rgbtBlue;
#endif // FREEIMAGE_COLORORDER
} FIE_RGBTRIPLE;

typedef struct {
#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR
      BYTE rgbBlue;
      BYTE rgbGreen;
      BYTE rgbRed;
#else
      BYTE rgbRed;
      BYTE rgbGreen;
      BYTE rgbBlue;
#endif // FREEIMAGE_COLORORDER
      BYTE rgbReserved;
} FIE_RGBQUAD;


/**
 * Extends FREE_IMAGE_FORMAT
 */
enum FIE_ImageFormat
{
    FIEF_FLO = FIF_JXR + 1,
    FIEF_SVG
};


/**
 * Must be called after FreeImage_Initialise()
 */
void FreeImageExt_Initialise();


/**
 * Does not call FreeImage_DeInitialise() internally.
 */
void FreeImageExt_DeInitialise();

/**
 * Returns a number of channels for color images, otherwise 1
 */
DWORD FreeImageExt_GetChannelsNumber(FIBITMAP* dib);


/**
 * Extended version of FreeImage_ConvertToFloat converting int16, uint16, int32, and uint32 as c-cast without normalization
 */
FIBITMAP* FreeImageExt_ConvertToFloat(FIBITMAP* dib);


enum FIE_ToneMapping
{
    FIETMO_DRAGO03    = ::FITMO_DRAGO03,
    FIETMO_REINHARD05 = ::FITMO_REINHARD05,
    FIETMO_FATTAL02   = ::FITMO_FATTAL02,
    FIETMO_NONE,        ///< Clamp
    FIETMO_LINEAR,      ///< Linear scale if possible, otherwise division by peak brightness with Y channel adjustment
};

FIBITMAP* FreeImageExt_ToneMapping(FIBITMAP* src, FIE_ToneMapping mode);

/**
 * Text name for tonemapping mode
 */
const char* FreeImageExt_TMtoString(FIE_ToneMapping mode);


enum FIE_AlphaFunction
{
    FIEAF_SrcAlpha ///< Use only src alpha, ignore dst alpha
};

/**
 * Draw 'src' bitmap onto 'dst' bitmap.
 * @param dst Destination bitmap (only 32bpp is supported for now).
 * @param src Source bitmap (only 32bpp is supported for now).
 * @param alpha Alpha function used for blending dst and src pixels.
 * @param left X coordinate of the 'src' image top left corner in 'dst' coordinates.
 * @param top Y coordinate of the 'src' image top left corner in 'dst' coordinates.
 * @return True on success
 */
BOOL FreeImageExt_Draw(FIBITMAP* dst, FIBITMAP* src, FIE_AlphaFunction alpha, int left FI_DEFAULT(0), int top FI_DEFAULT(0));


/**
 * Returns short image type description
 */
const char* FreeImageExt_DescribeImageType(FIBITMAP* dib);


#ifdef __cplusplus

using UniqueBitmap = std::unique_ptr<FIBITMAP, decltype(&::FreeImage_Unload)>;

template <typename Ty_>
inline
const Ty_& FreeImageExt_GetTagValue(FITAG* tag)
{
    return *static_cast<std::add_const_t<Ty_>*>(FreeImage_GetTagValue(tag));
}

template <typename Ty_>
inline
Ty_ FreeImageExt_GetMetadataValue(FREE_IMAGE_MDMODEL model, FIBITMAP* dib, const char* key, const Ty_& defaultVal)
{
    FITAG* tag = nullptr;
    const BOOL succ = FreeImage_GetMetadata(model, dib, key, &tag);
    if(succ && tag) {
        return *static_cast<std::add_const_t<Ty_>*>(FreeImage_GetTagValue(tag));
    }
    return defaultVal;
}

inline
BOOL FreeImageExt_SetMetadataValue(FREE_IMAGE_MDMODEL model, FIBITMAP* dib, const char* key, const float& val)
{
    std::unique_ptr<FITAG, decltype(&::FreeImage_DeleteTag)> tag(FreeImage_CreateTag(), &::FreeImage_DeleteTag);
    if (tag) {
        if (FreeImage_SetTagKey(tag.get(), key) &&
                FreeImage_SetTagLength(tag.get(), sizeof(float)) &&
                FreeImage_SetTagCount(tag.get(), 1) &&
                FreeImage_SetTagType(tag.get(), FIDT_FLOAT) &&
                FreeImage_SetTagValue(tag.get(), &val)) {
            return FreeImage_SetMetadata(model, dib, key, tag.get());
        }
    }
    return false;
}

#endif //__cplusplus



#endif // FREEIMAGEEXT_H

