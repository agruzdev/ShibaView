/**
* ShibaView
*
* The MIT License (MIT)
* Copyright (c) 2018 Alexey Gruzdev
*/

#include "BitmapSource.h"

BitmapSource::BitmapSource(const QString & filename, FREE_IMAGE_FORMAT fif)
{
#ifdef _WIN32
    const auto uniName = filename.toStdWString();
    mBitmap = FreeImage_LoadU(fif, uniName.c_str(), JPEG_EXIFROTATE);
#else
    const auto utfName = filename.toUtf8().toStdString();
    mBitmap = FreeImage_Load(fif, utfName.c_str(), JPEG_EXIFROTATE);
#endif
    if (nullptr == mBitmap) {
        throw std::runtime_error("BitmapSource[BitmapSource]: Failed to load file.");
    }
}

BitmapSource::~BitmapSource()
{
    FreeImage_Unload(mBitmap);
}

uint32_t BitmapSource::pagesCount() const Q_DECL_NOEXCEPT
{
    return 1;
}

FIBITMAP* BitmapSource::decodePage(uint32_t /*page*/, AnimationInfo* /*anim*/) Q_DECL_NOEXCEPT
{
    return mBitmap;
}

void BitmapSource::releasePage(FIBITMAP*) Q_DECL_NOEXCEPT
{
}

bool BitmapSource::storesResidual() const Q_DECL_NOEXCEPT
{
    return false;
}
