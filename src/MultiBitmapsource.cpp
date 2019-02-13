/**
* ShibaView
*
* The MIT License (MIT)
* Copyright (c) 2018 Alexey Gruzdev
*/

#include "MultiBitmapsource.h"

#include <limits>
#include <vector>
#include <fstream>


namespace
{
    template <typename Ty_>
    Ty_ FreeImage_GetMetadataValue(FREE_IMAGE_MDMODEL model, FIBITMAP *dib, const char *key, const Ty_ & defaultVal)
    {
        FITAG* tag = nullptr;
        const auto succ = FreeImage_GetMetadata(model, dib, key, &tag);
        if(succ && tag) {
            return *static_cast<std::add_const_t<Ty_>*>(FreeImage_GetTagValue(tag));
        }
        return defaultVal;
    }
}


// Internal buffer for FreeImage_OpenMultiBitmapU issue workaround
struct MultibitmapBuffer
{
    std::vector<unsigned char> data;
    FIMEMORY* stream = nullptr;
};


MultibitmapSource::MultibitmapSource(const QString & filename, FREE_IMAGE_FORMAT fif)
    : mFormat(fif)
{
#ifdef _WIN32
    const auto uniName = filename.toStdWString();
    // ToDo: Since FreeImage is missing FreeImage_OpenMultiBitmapU function, read through memory buffer
    std::ifstream file(uniName, std::ios::binary);
    if (file.is_open()) {
        mBuffer = std::make_unique<MultibitmapBuffer>();

        file.seekg(0, std::ios::end);
        mBuffer->data.resize(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(mBuffer->data.data()), mBuffer->data.size());
        file.close();

        if(mBuffer->data.size() > std::numeric_limits<DWORD>::max()) {
            throw std::runtime_error("MultibitmapSource[MultibitmapSource]: Input file is too big!");
        }

        mBuffer->stream = FreeImage_OpenMemory(mBuffer->data.data(), static_cast<DWORD>(mBuffer->data.size()));
        if (mBuffer->stream) {
            mMultibitmap = FreeImage_LoadMultiBitmapFromMemory(fif, mBuffer->stream, JPEG_EXIFROTATE);
        }
    }
#else
    const auto utfName = filename.toUtf8().toStdString();
    mMultibitmap = FreeImage_OpenMultiBitmap(fif, utfName.c_str(), FALSE, TRUE, FALSE, JPEG_EXIFROTATE);
#endif
    if (nullptr == mMultibitmap) {
        throw std::runtime_error("MultibitmapSource[MultibitmapSource]: Failed to load file.");
    }
}

MultibitmapSource::~MultibitmapSource()
{
    FreeImage_CloseMultiBitmap(mMultibitmap);
    if (mBuffer && mBuffer->stream) {
        FreeImage_CloseMemory(mBuffer->stream);
    }
}

uint32_t MultibitmapSource::pagesCount() const Q_DECL_NOEXCEPT
{
    return static_cast<uint32_t>(FreeImage_GetPageCount(mMultibitmap));
}

FIBITMAP* MultibitmapSource::decodePage(uint32_t pageIdx, AnimationInfo* anim) Q_DECL_NOEXCEPT
{
    const auto bmp = FreeImage_LockPage(mMultibitmap, static_cast<int>(pageIdx));
    if (bmp && anim) {
        anim->offsetX  = FreeImage_GetMetadataValue(FIMD_ANIMATION, bmp, "FrameLeft", 0);
        anim->offsetY  = FreeImage_GetMetadataValue(FIMD_ANIMATION, bmp, "FrameTop",  0);
        anim->duration = FreeImage_GetMetadataValue(FIMD_ANIMATION, bmp, "FrameTime", 0);
        anim->disposal = FreeImage_GetMetadataValue(FIMD_ANIMATION, bmp, "DisposalMethod", DisposalType::eLeave);
    }
    return bmp;
}

void MultibitmapSource::releasePage(FIBITMAP* page) Q_DECL_NOEXCEPT
{
    FreeImage_UnlockPage(mMultibitmap, page, false);
}

bool MultibitmapSource::storesResidual() const Q_DECL_NOEXCEPT
{
    return mFormat == FIF_GIF;
}
