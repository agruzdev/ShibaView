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

#ifndef FREEIMAGEEXT_H
#define FREEIMAGEEXT_H

#include "FreeImage.h"

enum FIE_ToneMapping
{
    FIETMO_DRAGO03    = ::FITMO_DRAGO03,
    FIETMO_REINHARD05 = ::FITMO_REINHARD05,
    FIETMO_FATTAL02   = ::FITMO_FATTAL02,
    FIETMO_NONE,
    FIETMO_LINEAR,
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

#endif // FREEIMAGEEXT_H
