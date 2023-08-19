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

#include "FreeImageExt.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <tuple>
#include "PluginFLO.h"
#include "PluginSVG.h"



void FreeImageExt_Initialise()
{
    FreeImage_RegisterLocalPlugin(&initPluginFLO);
    FreeImage_RegisterLocalPlugin(&initPluginSVG);
}


void FreeImageExt_DeInitialise()
{
}


const char* FreeImageExt_TMtoString(FREE_IMAGE_TMO mode)
{
    switch(mode) {
        case FITMO_CLAMP:
            return "None";
        case FITMO_LINEAR:
            return "Linear";
        case FITMO_DRAGO03:
            return "F.Drago, 2003";
        case FITMO_REINHARD05:
            return "E. Reinhard, 2005";
        case FITMO_FATTAL02:
            return "R. Fattal, 2002";
        default:
            return nullptr;
    }
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

        case FIT_RGBA32:
            return "RGBA32";

        case FIT_RGB32:
            return "RGB32";

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

        case FIT_COMPLEXF:
            return "Complex Float32";

        case FIT_COMPLEX:
            return "Complex Float64";

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


