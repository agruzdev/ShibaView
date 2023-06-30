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

#include "WindowsThumbnailProvider.h"

#ifdef _WIN32

#include <propsys.h>
#include <shlwapi.h>
#include <thumbcache.h>
#include <unknwn.h>
#include <fstream>
#include <memory>
#include <mutex>
#include <new>

#include "../ImageSource.h"
#include "../Player.h"
#include "../FreeImageExt.h"

#define ENABLE_LOG (0)

int filterException(int /*code*/, PEXCEPTION_POINTERS /*ex*/) {
    return EXCEPTION_EXECUTE_HANDLER;
}

class WindowsThumbnailProvider
    : public IInitializeWithFile
    , public IThumbnailProvider
{
public:
    WindowsThumbnailProvider()
    {
        static std::once_flag once;
        //std::call_once(once, []{ FreeImageExt_Initialise(); });
#if ENABLE_LOG
        std::locale::global(std::locale("Russian_Russia"));
        mLog.open("D:/ShibaThumbnailService.log", std::ios::app);
        mLog << L"Created" << std::endl;
#endif
    }

    WindowsThumbnailProvider(const WindowsThumbnailProvider&) = delete;

    WindowsThumbnailProvider(WindowsThumbnailProvider&&) = delete;

    ~WindowsThumbnailProvider()
    {
#if ENABLE_LOG
        mLog << L"Destroyed" << std::endl;
        mLog.close();
#endif
    }

    WindowsThumbnailProvider& operator=(const WindowsThumbnailProvider&) = delete;

    WindowsThumbnailProvider& operator=(WindowsThumbnailProvider&&) = delete;


    // IUnknown
    IFACEMETHODIMP_(ULONG) AddRef()
    {
#if ENABLE_LOG
        mLog << L"AddRef() is called" << std::endl;
#endif
        return InterlockedIncrement(&mRefs);
    }

    IFACEMETHODIMP_(ULONG) Release()
    {
#if ENABLE_LOG
        mLog << L"Release() is called" << std::endl;
#endif
        const ULONG cRef = InterlockedDecrement(&mRefs);
        if (!cRef) {
            delete this;
        }
        return cRef;
    }

    IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv)
    {
#if ENABLE_LOG
        mLog << L"QueryInterface() is called" << std::endl;
#endif
        static const QITAB qit[] = {
            QITABENT(WindowsThumbnailProvider, IThumbnailProvider),
            QITABENT(WindowsThumbnailProvider, IInitializeWithFile),
            { 0 },
        };
        return QISearch(this, qit, riid, ppv);
    }

    // IInitializeWithFile
    IFACEMETHODIMP Initialize(LPCWSTR pszFilePath, DWORD /*grfMode*/)
    {
#if ENABLE_LOG
        mLog << L"Initialize() is called = " << pszFilePath << std::endl;
#endif
        if (!pszFilePath) {
            return E_INVALIDARG;
        }
        if (!mFilePath.empty()) {
            return E_UNEXPECTED;
        }
        mFilePath = pszFilePath;
        return S_OK;
    }

    IFACEMETHODIMP GetThumbnail(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha)
    {
        __try {
            return GetThumbnail_(cx, phbmp, pdwAlpha);
        }
        __except(filterException(GetExceptionCode(), GetExceptionInformation())) {
        }
        return S_FALSE;
    }

    // IThumbnailProvider
    IFACEMETHODIMP GetThumbnail_(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha)
    {
        try {
#if ENABLE_LOG
            mLog << L"GetThumbnail() is called " << mFilePath << std::endl;
#endif
            auto bitmapSource = ImageSource::Load(QString::fromStdWString(mFilePath));
            if (!bitmapSource || bitmapSource->pagesCount() == 0) {
                throw std::runtime_error("Failed to open file");
            }
#if ENABLE_LOG
            mLog << L"File is opened " << mFilePath << std::endl;
#endif
            auto page = bitmapSource->lockPage(0);
            if (!page) {
                throw std::runtime_error("Failed to read file");
            }

#if ENABLE_LOG
            mLog << L"Page is locked " << mFilePath << std::endl;
#endif
            UniqueBitmap thumbnail = Player::getOrMakeThumbnail(page->getBitmap(), cx);
            if (!thumbnail) {
                throw std::runtime_error("Failed to make a thumbnail");
            }

#if ENABLE_LOG
            mLog << L"Got thumbnail " << mFilePath << std::endl;
#endif

            const uint32_t bmpHeight = FreeImage_GetHeight(thumbnail.get());
            const uint32_t bmpWidth  = FreeImage_GetWidth(thumbnail.get());
            const size_t   bmpStride = static_cast<size_t>(bmpWidth) * 4u;

            BITMAPINFO bmi{};
            bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
            bmi.bmiHeader.biWidth  = bmpWidth;
            bmi.bmiHeader.biHeight = bmpHeight;
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;

            BYTE *pBits = nullptr;
            std::unique_ptr<std::remove_pointer_t<HBITMAP>, decltype(&::DeleteObject)> bmp(CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&pBits), nullptr, 0), &::DeleteObject);
            if (!pBits) {
                throw std::runtime_error("Failed to CreateDIBSection()");
            }

#if ENABLE_LOG
            mLog << L"Allocated HBITMAP " << mFilePath << std::endl;
#endif

            WTS_ALPHATYPE resultAlpha = WTSAT_RGB;

            switch (FreeImage_GetBPP(thumbnail.get())) {
            case 1:
                for (uint32_t y = 0; y < bmpHeight; ++y) {
                    const auto dstLine = reinterpret_cast<RGBQUAD*>(pBits + y * bmpStride);
                    for (uint32_t x = 0; x < bmpWidth; ++x) {
                        BYTE val = 0;
                        FreeImage_GetPixelIndex(thumbnail.get(), x, y, &val);
                        val = val ? 255 : 0;
                        dstLine[x].rgbRed   = val;
                        dstLine[x].rgbGreen = val;
                        dstLine[x].rgbBlue  = val;
                    }
                }
                break;
            case 8:
                for (uint32_t y = 0; y < bmpHeight; ++y) {
                    const auto srcLine = reinterpret_cast<const BYTE*>(FreeImage_GetScanLine(thumbnail.get(), y));
                    const auto dstLine = reinterpret_cast<RGBQUAD*>(pBits + y * bmpStride);
                    for (uint32_t x = 0; x < bmpWidth; ++x) {
                        const BYTE val = srcLine[x];
                        dstLine[x].rgbRed   = val;
                        dstLine[x].rgbGreen = val;
                        dstLine[x].rgbBlue  = val;
                    }
                }
                break;
            case 24:
                for (uint32_t y = 0; y < bmpHeight; ++y) {
                    const auto srcLine = reinterpret_cast<const FIE_RGBTRIPLE*>(FreeImage_GetScanLine(thumbnail.get(), y));
                    const auto dstLine = reinterpret_cast<RGBQUAD*>(pBits + y * bmpStride);
                    for (uint32_t x = 0; x < bmpWidth; ++x) {
                        dstLine[x].rgbRed   = srcLine[x].rgbtRed;
                        dstLine[x].rgbGreen = srcLine[x].rgbtGreen;
                        dstLine[x].rgbBlue  = srcLine[x].rgbtBlue;
                    }
                }
                break;
            case 32:
                for (uint32_t y = 0; y < bmpHeight; ++y) {
                    const auto srcLine = reinterpret_cast<const FIE_RGBQUAD*>(FreeImage_GetScanLine(thumbnail.get(), y));
                    const auto dstLine = reinterpret_cast<RGBQUAD*>(pBits + y * bmpStride);
                    for (uint32_t x = 0; x < bmpWidth; ++x) {
                        dstLine[x].rgbRed       = srcLine[x].rgbRed;
                        dstLine[x].rgbGreen     = srcLine[x].rgbGreen;
                        dstLine[x].rgbBlue      = srcLine[x].rgbBlue;
                        dstLine[x].rgbReserved  = srcLine[x].rgbReserved;
                    }
                }
                resultAlpha = WTSAT_ARGB;
                break;
            default:
                throw std::logic_error("Internal image is 1, 8, 24 or 32 bit");
            }

#if ENABLE_LOG
            mLog << L"Copied HBITMAP " << mFilePath << std::endl;
#endif

            if (phbmp) {
#if ENABLE_LOG
                mLog << L"GetThumbnail() is OK" << std::endl;
#endif
                *phbmp = bmp.release();
                *pdwAlpha = resultAlpha;
                return S_OK;
            }
        }
#if ENABLE_LOG
        catch(std::exception& err) {
            mLog << err.what() << std::endl;
        }
#endif
        catch(...) {
        }
#if ENABLE_LOG
        mLog << L"GetThumbnail() is FAIL" << std::endl;
#endif
        return S_FALSE;
    }

private:
    ULONG mRefs = static_cast<ULONG>(1);
    std::wstring mFilePath;
#if ENABLE_LOG
    std::wofstream mLog;
#endif
};


HRESULT WindowsThumbnailProvider_CreateInstance(REFIID riid, void **ppv)
{
    WindowsThumbnailProvider* pNew = new(std::nothrow) WindowsThumbnailProvider();
    HRESULT hr = pNew ? S_OK : E_OUTOFMEMORY;
    if (SUCCEEDED(hr)) {
        hr = pNew->QueryInterface(riid, ppv);
        pNew->Release();
    }
    return hr;
}


#endif //_WIN32
