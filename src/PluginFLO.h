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

#ifndef PLUGINFLO_H
#define PLUGINFLO_H

#include "FreeImage.hpp"

class PluginFlo
    : public fi::Plugin2
{
public:
    PluginFlo();
    ~PluginFlo();

    const char* FormatProc() override;
    const char* DescriptionProc() override;
    const char* ExtensionListProc() override;
    FIBITMAP* LoadProc(FreeImageIO* io, fi_handle handle, uint32_t page, uint32_t flags, void* data) override;
    bool ValidateProc(FreeImageIO* io, fi_handle handle) override;


    static
    FIBITMAP* cvtFloToRgb(FIBITMAP* flo);
};


#endif // PLUGINFLO_H
