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


#include "RawSource.h"

#include <fstream>
#include <QInputDialog>
#include <QApplication>
#include "RawFormatDialog.h"
#include "RawFormatDialogWrapper.h"

namespace
{
    std::vector<BYTE> getBinaryFileContent(const QString & filename)
    {
        std::vector<BYTE> bytes;
        std::unique_ptr<FILE, decltype(&::fclose)> f(nullptr, &::fclose);
#ifdef _WIN32
        f.reset(_wfopen(filename.toStdWString().c_str(), L"rb"));
#else
        f.reset(::fopen(filename.toUtf8().toStdString().c_str(), "rb"));
#endif

        if (0 != ::fseek(f.get(), 0, SEEK_END)) {
            return bytes;
        }

        const size_t size = static_cast<size_t>(::ftell(f.get()));

        if (0 != ::fseek(f.get(), 0, SEEK_SET)) {
            return bytes;
        }

        bytes.resize(size);
        if (size != ::fread(bytes.data(), 1, size, f.get())) {
            bytes.clear();
        }

        return bytes;
    }

    FREE_IMAGE_TYPE selectImageType(RawPixelColor colorSpace, RawDataType dataType)
    {
        static std::map<std::pair<RawPixelColor, RawDataType>, FREE_IMAGE_TYPE> sKnownImageTypes = {
            { std::make_pair(RawPixelColor::eY, RawDataType::eFloat64), FIT_DOUBLE },
            { std::make_pair(RawPixelColor::eY, RawDataType::eFloat32), FIT_FLOAT },
            { std::make_pair(RawPixelColor::eY, RawDataType::eUInt32), FIT_UINT32 },
            { std::make_pair(RawPixelColor::eY, RawDataType::eUInt16), FIT_UINT16 },
            { std::make_pair(RawPixelColor::eY, RawDataType::eUInt8), FIT_BITMAP },
            { std::make_pair(RawPixelColor::eRGB, RawDataType::eFloat32), FIT_RGBF },
            { std::make_pair(RawPixelColor::eRGB, RawDataType::eUInt16), FIT_RGB16 },
            { std::make_pair(RawPixelColor::eRGB, RawDataType::eUInt8), FIT_BITMAP },
            { std::make_pair(RawPixelColor::eRGBA, RawDataType::eFloat32), FIT_RGBAF },
            { std::make_pair(RawPixelColor::eRGBA, RawDataType::eUInt16), FIT_RGBA16 },
            { std::make_pair(RawPixelColor::eRGBA, RawDataType::eUInt8), FIT_BITMAP },
        };
        const auto & knownTypes = sKnownImageTypes;
        const auto it = knownTypes.find(std::make_pair(colorSpace, dataType));
        return (it != knownTypes.cend()) ? (*it).second : FIT_UNKNOWN;
    }

}

RawSource::RawSource(const QString & filename)
{
    bool ok = false;
    RawFormat format{};

    std::tie(ok, format) = RawFromatDialogWrapper::showDialog();
    if (!ok) {
        throw std::runtime_error("RawSource[ctor]: Cancelled by user.");
    }

    const std::vector<BYTE> fileContent = getBinaryFileContent(filename);

    const FREE_IMAGE_TYPE fit = selectImageType(format.colorSpace, format.dataType);
    if (fit == FIT_UNKNOWN) {
        throw std::runtime_error("RawSource[ctor]: Unsupported format.");
    }

    const size_t channels = getChannelsNumber(format.colorSpace);
    const size_t bytesPerPixel  = channels * getSizeOfDataType(format.dataType);
    const size_t totalImageSize = format.width * format.height * bytesPerPixel;

    if (fileContent.size() < totalImageSize) {
        throw std::runtime_error("RawSource[ctor]: Invalid file size.");
    }

    mBitmap = FreeImage_AllocateT(fit, format.width, format.height, 8 * bytesPerPixel);
    BYTE* dstLine = FreeImage_GetScanLine(mBitmap, format.height - 1);
    const BYTE* srcLine = fileContent.data();
    const size_t dstStride = FreeImage_GetPitch(mBitmap);
    const size_t srcStride = format.width * bytesPerPixel;
    for (uint32_t y = 0; y < format.height; ++y, dstLine -= dstStride, srcLine += srcStride) {
        std::memcpy(dstLine, srcLine, srcStride);
    }
}

RawSource::~RawSource()
{
    if (mBitmap) {
        FreeImage_Unload(mBitmap);
    }
}

uint32_t RawSource::doPagesCount() const Q_DECL_NOEXCEPT
{
    return 1;
}

FIBITMAP* RawSource::doDecodePage(uint32_t /*page*/, AnimationInfo* /*anim*/) Q_DECL_NOEXCEPT
{
    return mBitmap;
}

void RawSource::doReleasePage(FIBITMAP*) Q_DECL_NOEXCEPT
{
}

bool RawSource::doStoresDifference() const Q_DECL_NOEXCEPT
{
    return false;
}
