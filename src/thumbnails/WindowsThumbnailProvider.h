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

#ifndef WINDOWS_THUMBNAIL_PROVIDER_H_
#define WINDOWS_THUMBNAIL_PROVIDER_H_

#ifdef _WIN32

#define NOMINMAX
#define WIN_LEAN_AND_MEAN
#include <unknwn.h>

HRESULT WindowsThumbnailProvider_CreateInstance(REFIID riid, void **ppv);

STDAPI DllRegisterServer();

STDAPI DllUnregisterServer();

#endif //_WIN32

#endif //WINDOWS_THUMBNAIL_PROVIDER_H_