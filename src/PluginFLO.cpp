/**
 * @file
 *
 * Copyright 2018-2026 Alexey Gruzdev
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
#include "FreeImageExt.h"

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

        FIRGB8 computeColor(float fx, float fy) const
        {
            const float rad = std::sqrt(fx * fx + fy * fy);
            const float a = std::atan2(-fy, -fx) / M_PI;
            const float fk = (a + 1.0) / 2.0 * (kMaxColors - 1);
            const int k0 = static_cast<int>(fk);
            const int k1 = (k0 + 1) % kMaxColors;
            float f = fk - k0;
            uint8_t pix[3] = {};
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
            FIRGB8 rgb;
            rgb.red   = pix[2];
            rgb.green = pix[1];
            rgb.blue  = pix[0];
            return rgb;
        }

    private:
        ColorWheel()
        {
            // relative lengths of color transitions:
            // these are chosen based on perceptual similarity
            // (e.g. one can distinguish more shades between red and yellow 
            //  than between yellow and green)
            constexpr uint32_t RY = 15;
            constexpr uint32_t YG = 6;
            constexpr uint32_t GC = 4;
            constexpr uint32_t CB = 11;
            constexpr uint32_t BM = 13;
            constexpr uint32_t MR = 6;
            static_assert(RY + YG + GC + CB + BM + MR == kMaxColors, "Invalid colors number");

            size_t k = 0;
            for (uint32_t i = 0; i < RY; i++) {
                setColor(255, 255 * i / RY, 0, k++);
            }
            for (uint32_t i = 0; i < YG; i++) {
                setColor(255 - 255 * i / YG, 255, 0, k++);
            }
            for (uint32_t i = 0; i < GC; i++) {
                setColor(0, 255, 255 * i / GC, k++);
            }
            for (uint32_t i = 0; i < CB; i++) {
                setColor(0, 255 - 255 * i / CB, 255, k++);
            }
            for (uint32_t i = 0; i < BM; i++) {
                setColor(255 * i / BM, 0, 255, k++);
            }
            for (uint32_t i = 0; i < MR; i++) {
                setColor(255, 0, 255 - 255 * i / MR, k++);
            }
        }

        void setColor(uint32_t r, uint32_t g, uint32_t b, size_t k)
        {
            assert(0 <= r && r <= 255);
            assert(0 <= g && g <= 255);
            assert(0 <= b && b <= 255);
            mColors[k][0] = static_cast<uint8_t>(r);
            mColors[k][1] = static_cast<uint8_t>(g);
            mColors[k][2] = static_cast<uint8_t>(b);
        }

        ~ColorWheel() = default;

        std::array<uint8_t[3], kMaxColors> mColors;
    };

    bool isUnknownFlow(float u, float v)
    {
        return (std::fabs(u) >  UNKNOWN_FLOW_THRESH) || (std::fabs(v) >  UNKNOWN_FLOW_THRESH) || std::isnan(u) || std::isnan(v);
    }

    template <typename PixelTy_>
    FIBITMAP* cvtFloToRgbImpl(FIBITMAP* flo)
    {
        const uint32_t width  = FreeImage_GetWidth(flo);
        const uint32_t height = FreeImage_GetHeight(flo);

        if (width * height <= 0) {
            return nullptr;
        }

        const float maxrad = FreeImageExt_GetMetadataValue<float>(FIMD_CUSTOM, flo, "Max R", 1.0f);

        std::unique_ptr<FIBITMAP, decltype(&::FreeImage_Unload)> rgb(FreeImage_Allocate(width, height, 24), &::FreeImage_Unload);

        for (uint32_t y = 0; y < height; y++) {
            const auto flowLine = reinterpret_cast<PixelTy_*>(FreeImage_GetScanLine(flo, y));
            const auto rgbLine  = reinterpret_cast<FIRGB8*>(FreeImage_GetScanLine(rgb.get(), y));
            for (uint32_t x = 0; x < width; ++x) {
                const auto fx = flowLine[x].r;
                const auto fy = flowLine[x].i;
                if (!isUnknownFlow(fx, fy)) {
                    rgbLine[x] = ColorWheel::getInstance().computeColor(static_cast<float>(fx / maxrad), static_cast<float>(fy / maxrad));
                }
                else {
                    std::memset(rgbLine + x, 0, sizeof(FIRGB8));
                }
            }
        }

        return rgb.release();
    }

} // namespace



// Reference code:
// https://vision.middlebury.edu/flow/code/flow-code/flowIO.cpp
PluginFlo::PluginFlo() = default;

PluginFlo::~PluginFlo() = default;

const char* PluginFlo::FormatProc() {
    return "FLO";
}

const char* PluginFlo::DescriptionProc() {
    return "File format used for optical flow. Reference: https://vision.middlebury.edu";
}

const char* PluginFlo::ExtensionListProc() {
    return "flo";
}

FIBITMAP* PluginFlo::LoadProc(FreeImageIO* io, fi_handle handle, uint32_t /*page*/, uint32_t /*flags*/, void* /*data*/) {

    float tag = 0.0f;
    uint32_t width = 0;
    uint32_t height = 0;

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

    std::unique_ptr<FIBITMAP, decltype(&::FreeImage_Unload)> flowImage(FreeImage_AllocateT(FIT_COMPLEXF, width, height, 8 * sizeof(FICOMPLEXF)), &::FreeImage_Unload);

    const int flowLineSize = 2 * width; // two components
    for (uint32_t y = 0; y < height; y++) {
        const auto flowLine = reinterpret_cast<FICOMPLEXF*>(FreeImage_GetScanLine(flowImage.get(), height - 1 - y));
        if (io->read_proc(flowLine, sizeof(float), flowLineSize, handle) != flowLineSize) {
            qDebug() << "PluginFLO[Load]: file is too short";
            return nullptr;
        }
        for (uint32_t x = 0; x < width; ++x) {
            const float fx = flowLine[x].r;
            const float fy = flowLine[x].i;
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
    FreeImageExt_SetMetadataValue(FIMD_CUSTOM, flowImage.get(), "ImageType", "2D motion vector");

    return flowImage.release();
}

bool PluginFlo::ValidateProc(FreeImageIO* io, fi_handle handle) {
    float tag = 0.0f;
    if (io->read_proc(&tag, sizeof(tag), 1, handle) != 1) {
        return false;
    }
    return (tag == TAG_FLOAT);
};


FIBITMAP* PluginFlo::cvtFloToRgb(FIBITMAP* flo)
{
    if (!FreeImage_HasPixels(flo)) {
        return nullptr;
    }

    switch (FreeImage_GetImageType(flo)) {
    case FIT_COMPLEXF:
        return cvtFloToRgbImpl<FICOMPLEXF>(flo);
    case FIT_COMPLEX:
        return cvtFloToRgbImpl<FICOMPLEX>(flo);
    default:
        return nullptr;
    }
}


//FREE_IMAGE_FORMAT PluginFlo::getRegisteredId()
//{
//    static auto id = fi::Plugin2::RegisterLocal(std::make_unique<PluginFlo>());
//    return static_cast<FREE_IMAGE_FORMAT>(id);
//}
