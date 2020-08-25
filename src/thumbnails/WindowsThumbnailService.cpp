/**
 * @file
 *
 * Copyright 2018-2020 Alexey Gruzdev
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

#include <array>
#include <objbase.h>
#include <shlwapi.h>
#include <thumbcache.h> // For IThumbnailProvider.
#include <shlobj.h>     // For SHChangeNotify
#include <new>
#include <vector>

#include "../Global.h"

#define SZ_ITHUMBNAILPROVIDER_SHELLEXTENSION  L"{E357FCCD-A995-4576-B01F-234630154E96}"

#define SZ_CLSID_SHIBATHUMBHANDLER     L"{7DB3DA20-E0EA-49EB-BDA4-9A75B9D38220}"
#define SZ_SHIBATHUMBHANDLER           L"ShibaView Thumbnail Handler"

// {7DB3DA20-E0EA-49EB-BDA4-9A75B9D38220}
static const CLSID CLSID_ShibaThumbHandler = { 0x7db3da20, 0xe0ea, 0x49eb, { 0xbd, 0xa4, 0x9a, 0x75, 0xb9, 0xd3, 0x82, 0x20 } };



typedef HRESULT (*PFNCREATEINSTANCE)(REFIID riid, void **ppvObject);
struct CLASS_OBJECT_INIT
{
    const CLSID *pClsid;
    PFNCREATEINSTANCE pfnCreate;
};

// add classes supported by this module here
const CLASS_OBJECT_INIT c_rgClassObjectInit[] =
{
    { &CLSID_ShibaThumbHandler, WindowsThumbnailProvider_CreateInstance }
};


long g_cRefModule = 0;

// Handle the the DLL's module
HINSTANCE g_hInst = NULL;

// Standard DLL functions
STDAPI_(BOOL) DllMain(HINSTANCE hInstance, DWORD dwReason, void *)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hInst = hInstance;
        DisableThreadLibraryCalls(hInstance);
    }
    return TRUE;
}

STDAPI DllCanUnloadNow()
{
    // Only allow the DLL to be unloaded after all outstanding references have been released
    return (g_cRefModule == 0) ? S_OK : S_FALSE;
}

void DllAddRef()
{
    InterlockedIncrement(&g_cRefModule);
}

void DllRelease()
{
    InterlockedDecrement(&g_cRefModule);
}

class CClassFactory : public IClassFactory
{
public:
    static HRESULT CreateInstance(REFCLSID clsid, const CLASS_OBJECT_INIT *pClassObjectInits, size_t cClassObjectInits, REFIID riid, void **ppv)
    {
        *ppv = NULL;
        HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;
        for (size_t i = 0; i < cClassObjectInits; i++)
        {
            if (clsid == *pClassObjectInits[i].pClsid)
            {
                IClassFactory *pClassFactory = new (std::nothrow) CClassFactory(pClassObjectInits[i].pfnCreate);
                hr = pClassFactory ? S_OK : E_OUTOFMEMORY;
                if (SUCCEEDED(hr))
                {
                    hr = pClassFactory->QueryInterface(riid, ppv);
                    pClassFactory->Release();
                }
                break; // match found
            }
        }
        return hr;
    }

    CClassFactory(PFNCREATEINSTANCE pfnCreate) : _cRef(1), _pfnCreate(pfnCreate)
    {
        DllAddRef();
    }

    // IUnknown
    IFACEMETHODIMP QueryInterface(REFIID riid, void ** ppv)
    {
        static const QITAB qit[] =
        {
            QITABENT(CClassFactory, IClassFactory),
            { 0 }
        };
        return QISearch(this, qit, riid, ppv);
    }

    IFACEMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&_cRef);
    }

    IFACEMETHODIMP_(ULONG) Release()
    {
        long cRef = InterlockedDecrement(&_cRef);
        if (cRef == 0)
        {
            delete this;
        }
        return cRef;
    }

    // IClassFactory
    IFACEMETHODIMP CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
    {
        return punkOuter ? CLASS_E_NOAGGREGATION : _pfnCreate(riid, ppv);
    }

    IFACEMETHODIMP LockServer(BOOL fLock)
    {
        if (fLock)
        {
            DllAddRef();
        }
        else
        {
            DllRelease();
        }
        return S_OK;
    }

private:
    ~CClassFactory()
    {
        DllRelease();
    }

    long _cRef;
    PFNCREATEINSTANCE _pfnCreate;
};

STDAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **ppv)
{
    return CClassFactory::CreateInstance(clsid, c_rgClassObjectInit, ARRAYSIZE(c_rgClassObjectInit), riid, ppv);
}

// A struct to hold the information required for a registry entry

struct REGISTRY_ENTRY
{
    HKEY   hkeyRoot;
    PCWSTR pszKeyName;
    PCWSTR pszValueName;
    DWORD  dwType;
    PCWSTR pszData;
};

// Creates a registry key (if needed) and sets the default value of the key

HRESULT CreateRegKeyAndSetValue(const REGISTRY_ENTRY *pRegistryEntry)
{
    HKEY hKey;
    HRESULT hr = HRESULT_FROM_WIN32(RegCreateKeyExW(pRegistryEntry->hkeyRoot, pRegistryEntry->pszKeyName,
                                0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL));
    if (SUCCEEDED(hr))
    {
        hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, pRegistryEntry->pszValueName, 0, pRegistryEntry->dwType,
                            (LPBYTE) pRegistryEntry->pszData,
                            ((DWORD) wcslen(pRegistryEntry->pszData) + 1) * sizeof(WCHAR)));
        RegCloseKey(hKey);
    }
    return hr;
}

//
// Registers this COM server
//
STDAPI DllRegisterServer()
{
    HRESULT hr;

    WCHAR szModuleName[MAX_PATH];

    if (!GetModuleFileNameW(g_hInst, szModuleName, ARRAYSIZE(szModuleName)))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {
        // List of registry entries we want to create
        std::array<REGISTRY_ENTRY, 4> rgRegistryEntries = {
            // RootKey            KeyName                                                                                   ValueName                      Type         Data
            REGISTRY_ENTRY{HKEY_CURRENT_USER,   L"Software\\Classes\\CLSID\\" SZ_CLSID_SHIBATHUMBHANDLER,                                 NULL,                          REG_SZ,      SZ_SHIBATHUMBHANDLER},
            REGISTRY_ENTRY{HKEY_CURRENT_USER,   L"Software\\Classes\\CLSID\\" SZ_CLSID_SHIBATHUMBHANDLER L"\\InprocServer32",             NULL,                          REG_SZ,      szModuleName},
            REGISTRY_ENTRY{HKEY_CURRENT_USER,   L"Software\\Classes\\CLSID\\" SZ_CLSID_SHIBATHUMBHANDLER L"\\InprocServer32",             L"ThreadingModel",             REG_SZ,      L"Apartment"},
            REGISTRY_ENTRY{HKEY_CURRENT_USER,   L"Software\\Classes\\CLSID\\" SZ_CLSID_SHIBATHUMBHANDLER,                                 L"DisableProcessIsolation",    REG_DWORD,   L"\u0001"},
            //{HKEY_CURRENT_USER,   L"Software\\Classes\\.jpg\\ShellEx\\" SZ_ITHUMBNAILPROVIDER_SHELLEXTENSION,               NULL,                          REG_SZ,      SZ_CLSID_SHIBATHUMBHANDLER},
        };

        hr = S_OK;
        for (int i = 0; (i < rgRegistryEntries.size()) && SUCCEEDED(hr); i++) {
            hr = CreateRegKeyAndSetValue(&rgRegistryEntries[i]);
        }

         for (const QString& ext : Global::getSupportedExtensions()) {
            const auto key = L"Software\\Classes\\" + ext.toStdWString() + L"\\ShellEx\\" SZ_ITHUMBNAILPROVIDER_SHELLEXTENSION;

            REGISTRY_ENTRY entry{};
            entry.hkeyRoot = HKEY_CURRENT_USER;
            entry.pszKeyName = key.c_str();
            entry.pszValueName = nullptr;
            entry.dwType = REG_SZ;
            entry.pszData = SZ_CLSID_SHIBATHUMBHANDLER;

            hr = CreateRegKeyAndSetValue(&entry);
            if (!SUCCEEDED(hr)) {
                break;
            }
        }
    }
    if (SUCCEEDED(hr))
    {
        // This tells the shell to invalidate the thumbnail cache.
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
    }
    return hr;
}

//
// Unregisters this COM server
//
STDAPI DllUnregisterServer()
{
    HRESULT hr = S_OK;

    const std::array<PCWSTR, 1> rgpszKeys = {
        L"Software\\Classes\\CLSID\\" SZ_CLSID_SHIBATHUMBHANDLER,
        //L"Software\\Classes\\.jpg\\ShellEx\\" SZ_ITHUMBNAILPROVIDER_SHELLEXTENSION
    };

    // Delete the registry entries
    for (int i = 0; (i < rgpszKeys.size()) && SUCCEEDED(hr); i++) {
        hr = HRESULT_FROM_WIN32(RegDeleteTreeW(HKEY_CURRENT_USER, rgpszKeys[i]));
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
            // If the registry entry has already been deleted, say S_OK.
            hr = S_OK;
        }
    }

     for (const QString& ext : Global::getSupportedExtensions()) {
        const auto key = L"Software\\Classes\\" + ext.toStdWString() + "\\ShellEx\\" SZ_ITHUMBNAILPROVIDER_SHELLEXTENSION;
        hr = HRESULT_FROM_WIN32(RegDeleteTreeW(HKEY_CURRENT_USER, key.c_str()));
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
            // If the registry entry has already been deleted, say S_OK.
            hr = S_OK;
        }
    }

    return hr;
}


#endif //_WIN32
