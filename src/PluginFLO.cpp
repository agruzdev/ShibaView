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

#include "PluginFLO.h"

#ifndef _USE_MATH_DEFINES
# define _USE_MATH_DEFINES
#endif
#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif

#include <array>
#include <cassert>
#include <cmath>
#include <limits>
#include <vector>
#include <QDebug>



// first four bytes, should be the same in little endian
#define TAG_FLOAT 202021.25  // check for this when READING the file
#define TAG_STRING "PIEH"    // use this when WRITING the file
#define UNKNOWN_FLOW_THRESH 1e9

namespace
{

    // Reference code:
    // https://vision.middlebury.edu/flow/code/flow-code/colorcode.cpp
    class ColorWheel
    {
    public:
        static constexpr size_t kMaxColors = 55;

        static
        ColorWheel& getInstance()
        {
            static ColorWheel instance;
            return instance;
        }

        FIE_RGBTRIPLE computeColor(float fx, float fy) const
        {
            const float rad = std::sqrt(fx * fx + fy * fy);
            const float a = std::atan2(-fy, -fx) / M_PI;
            const float fk = (a + 1.0) / 2.0 * (kMaxColors - 1);
            const int k0 = static_cast<int>(fk);
            const int k1 = (k0 + 1) % kMaxColors;
            float f = fk - k0;
            BYTE pix[3] = {};
            for (int b = 0; b < 3; b++) {
                const float col0 = mColors[k0][b] / 255.0;
                const float col1 = mColors[k1][b] / 255.0;
                float col = (1 - f) * col0 + f * col1;
                if (rad <= 1) {
                    col = 1 - rad * (1 - col); // increase saturation with radius
                }
                else {
                    col *= 0.75; // out of range
                }
                pix[2 - b] = (int)(255.0 * col);
            }
            FIE_RGBTRIPLE rgb;
            rgb.rgbtRed = pix[2];
            rgb.rgbtGreen = pix[1];
            rgb.rgbtBlue = pix[0];
            return rgb;
        }

    private:
        ColorWheel()
        {
            // relative lengths of color transitions:
            // these are chosen based on perceptual similarity
            // (e.g. one can distinguish more shades between red and yellow 
            //  than between yellow and green)
            constexpr DWORD RY = 15;
            constexpr DWORD YG = 6;
            constexpr DWORD GC = 4;
            constexpr DWORD CB = 11;
            constexpr DWORD BM = 13;
            constexpr DWORD MR = 6;
            static_assert(RY + YG + GC + CB + BM + MR == kMaxColors, "Invalid colors number");

            size_t k = 0;
            for (DWORD i = 0; i < RY; i++) {
                setColor(255, 255 * i / RY, 0, k++);
            }
            for (DWORD i = 0; i < YG; i++) {
                setColor(255 - 255 * i / YG, 255, 0, k++);
            }
            for (DWORD i = 0; i < GC; i++) {
                setColor(0, 255, 255 * i / GC, k++);
            }
            for (DWORD i = 0; i < CB; i++) {
                setColor(0, 255 - 255 * i / CB, 255, k++);
            }
            for (DWORD i = 0; i < BM; i++) {
                setColor(255 * i / BM, 0, 255, k++);
            }
            for (DWORD i = 0; i < MR; i++) {
                setColor(255, 0, 255 - 255 * i / MR, k++);
            }
        }

        void setColor(DWORD r, DWORD g, DWORD b, size_t k)
        {
            assert(0 <= r && r <= 255);
            assert(0 <= g && g <= 255);
            assert(0 <= b && b <= 255);
            mColors[k][0] = static_cast<BYTE>(r);
            mColors[k][1] = static_cast<BYTE>(g);
            mColors[k][2] = static_cast<BYTE>(b);
        }

        ~ColorWheel() = default;

        std::array<BYTE[3], kMaxColors> mColors;
    };

    bool isUnknownFlow(float u, float v)
    {
        return (std::fabs(u) >  UNKNOWN_FLOW_THRESH) || (std::fabs(v) >  UNKNOWN_FLOW_THRESH) || std::isnan(u) || std::isnan(v);
    }

} // namespace

// Reference code:
// https://vision.middlebury.edu/flow/code/flow-code/flowIO.cpp
void initPluginFLO(Plugin *plugin, int format_id)
{
    (void)format_id;
    assert(FIEF_FLO == format_id);

    plugin->format_proc = []() -> const char* {
        return "FLO";
    };

    plugin->description_proc = []() -> const char* {
        return "File format used for optical flow. Reference: https://vision.middlebury.edu";
    };

    plugin->extension_proc = []() -> const char* {
        return "flo";
    };

    plugin->load_proc = [](FreeImageIO* io, fi_handle handle, int /*page*/, int /*flags*/, void* /*data*/) -> FIBITMAP* {

        float tag = 0.0f;
        DWORD width = 0;
        DWORD height = 0;

        if (io->read_proc(&tag, sizeof(tag), 1, handle) != 1 ||
                io->read_proc(&width, sizeof(width), 1, handle) != 1 ||
                io->read_proc(&height, sizeof(height), 1, handle) != 1) {
            qDebug() << "PluginFLO[Load]: failed to read the file header";
            return nullptr;
        }

        if (tag != TAG_FLOAT) {
            qDebug() << "PluginFLO[Load]: wrong tag";
            return nullptr;
        }

        if (width < 1 || width > 99999) {
            qDebug() << "PluginFLO[Load]: illegal width " << width;
            return nullptr;
        }

        if (height < 1 || height > 99999) {
            qDebug() << "PluginFLO[Load]: illegal height " << height;
            return nullptr;
        }

        float maxx = std::numeric_limits<float>::lowest();
        float maxy = std::numeric_limits<float>::lowest();
        float minx = std::numeric_limits<float>::max();
        float miny = std::numeric_limits<float>::max();
        float maxrad = -1.0f;

        const uint32_t flowLineSize = 2 * static_cast<uint32_t>(width); // two components

        std::unique_ptr<FIBITMAP, decltype(&::FreeImage_Unload)> flowImage(FreeImage_AllocateT(FIT_FLOAT, flowLineSize, height, 32), &::FreeImage_Unload);

        for (DWORD y = 0; y < height; y++) {
            float* flowLine = reinterpret_cast<float*>(FreeImage_GetScanLine(flowImage.get(), height - 1 - y));
            if (io->read_proc(flowLine, sizeof(float), flowLineSize, handle) != flowLineSize) {
                qDebug() << "PluginFLO[Load]: file is too short";
                return nullptr;
            }
            for (DWORD x = 0; x < width; ++x) {
                const DWORD x2 = x << 1;
                const float fx = flowLine[x2];
                const float fy = flowLine[x2 + 1];
                if (!isUnknownFlow(fx, fy)) {
                    maxx = std::max(maxx, fx);
                    maxy = std::max(maxy, fy);
                    minx = std::min(minx, fx);
                    miny = std::min(miny, fy);
                    maxrad = std::max(maxrad, std::sqrt(fx * fx + fy * fy));
                }
            }
        }

        if (maxrad == 0) { // if flow == 0 everywhere
            maxrad = 1;
        }

        FreeImageExt_SetMetadataValue(FIMD_CUSTOM, flowImage.get(), "Min X", minx);
        FreeImageExt_SetMetadataValue(FIMD_CUSTOM, flowImage.get(), "Max X", maxx);
        FreeImageExt_SetMetadataValue(FIMD_CUSTOM, flowImage.get(), "Min Y", miny);
        FreeImageExt_SetMetadataValue(FIMD_CUSTOM, flowImage.get(), "Max Y", maxy);
        FreeImageExt_SetMetadataValue(FIMD_CUSTOM, flowImage.get(), "Max R", maxrad);

        return flowImage.release();
    };

    plugin->save_proc = [](FreeImageIO* /*io*/, FIBITMAP* /*dib*/, fi_handle /*handle*/, int /*page*/, int /*flags*/, void* /*data*/) -> BOOL {
        return FALSE;
    };

    plugin->validate_proc = [](FreeImageIO* io, fi_handle handle) -> BOOL {

        float tag = 0.0f;
        if (io->read_proc(&tag, sizeof(tag), 1, handle) != 1) {
            return FALSE;
        }

        return (tag == TAG_FLOAT);
    };

}

FIBITMAP* cvtFloToRgb(FIBITMAP* flo)
{
    if (!flo) {
        return nullptr;
    }
    const DWORD width  = FreeImage_GetWidth(flo) / 2;
    const DWORD height = FreeImage_GetHeight(flo);

    if (width * height <= 0) {
        return nullptr;
    }

    const float maxrad = FreeImageExt_GetMetadataValue<float>(FIMD_CUSTOM, flo, "Max R", 1.0f);

    std::unique_ptr<FIBITMAP, decltype(&::FreeImage_Unload)> rgb(FreeImage_Allocate(width, height, 24), &::FreeImage_Unload);

    for (DWORD y = 0; y < height; y++) {
        const auto flowLine = reinterpret_cast<const float*>(FreeImage_GetScanLine(flo, y));
        const auto rgbLine  = reinterpret_cast<FIE_RGBTRIPLE*>(FreeImage_GetScanLine(rgb.get(), y));
        for (DWORD x = 0; x < width; ++x) {
            const DWORD x2 = x << 1;
            const float fx = flowLine[x2];
            const float fy = flowLine[x2 + 1];
            if (!isUnknownFlow(fx, fy)) {
                rgbLine[x] = ColorWheel::getInstance().computeColor(fx / maxrad, fy / maxrad);
            }
            else {
                std::memset(rgbLine + x, 0, sizeof(FIE_RGBTRIPLE));
            }
        }
    }

    return rgb.release();
}

