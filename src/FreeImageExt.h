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

#ifndef FREEIMAGEEXT_H
#define FREEIMAGEEXT_H

#ifdef __cplusplus
# include <type_traits>
# include <memory>
# include <string>
#endif

#include "FreeImage.h"



/**
 * Returns short image type description
 */
const char* FreeImageExt_DescribeImageType(FIBITMAP* dib);

const char* FreeImageExt_TMtoString(FREE_IMAGE_TMO mode);


#ifdef __cplusplus

using UniqueBitmap = std::unique_ptr<FIBITMAP, decltype(&::FreeImage_Unload)>;

template <typename Ty_>
inline
const Ty_& FreeImageExt_GetTagValue(FITAG* tag)
{
    return *static_cast<std::add_const_t<Ty_>*>(FreeImage_GetTagValue(tag));
}

template <typename Ty_>
inline
Ty_ FreeImageExt_GetMetadataValue(FREE_IMAGE_MDMODEL model, FIBITMAP* dib, const char* key, Ty_&& defaultVal)
{
    FITAG* tag = nullptr;
    const auto succ = FreeImage_GetMetadata(model, dib, key, &tag);
    if(succ && tag) {
        return *static_cast<std::add_const_t<Ty_>*>(FreeImage_GetTagValue(tag));
    }
    return std::move(defaultVal);
}

template <>
inline
std::string FreeImageExt_GetMetadataValue<std::string>(FREE_IMAGE_MDMODEL model, FIBITMAP* dib, const char* key, std::string&& defaultVal)
{
    FITAG* tag = nullptr;
    const auto succ = FreeImage_GetMetadata(model, dib, key, &tag);
    if (succ && tag) {
        const char* pchar = FreeImage_TagToString(model, tag);
        if (pchar) {
            return std::string(pchar);
        }
    }
    return std::move(defaultVal);
}

inline
bool FreeImageExt_SetMetadataValue(FREE_IMAGE_MDMODEL model, FIBITMAP* dib, const char* key, const float& val)
{
    std::unique_ptr<FITAG, decltype(&::FreeImage_DeleteTag)> tag(FreeImage_CreateTag(), &::FreeImage_DeleteTag);
    if (tag) {
        if (FreeImage_SetTagKey(tag.get(), key) &&
                FreeImage_SetTagLength(tag.get(), sizeof(float)) &&
                FreeImage_SetTagCount(tag.get(), 1) &&
                FreeImage_SetTagType(tag.get(), FIDT_FLOAT) &&
                FreeImage_SetTagValue(tag.get(), &val)) {
            return FreeImage_SetMetadata(model, dib, key, tag.get());
        }
    }
    return false;
}

inline
bool FreeImageExt_SetMetadataValue(FREE_IMAGE_MDMODEL model, FIBITMAP* dib, const char* key, const std::string& val)
{
    std::unique_ptr<FITAG, decltype(&::FreeImage_DeleteTag)> tag(FreeImage_CreateTag(), &::FreeImage_DeleteTag);
    if (tag) {
        if (FreeImage_SetTagKey(tag.get(), key) &&
                FreeImage_SetTagLength(tag.get(), val.size() + 1) &&
                FreeImage_SetTagCount(tag.get(), val.size() + 1) &&
                FreeImage_SetTagType(tag.get(), FIDT_ASCII) &&
                FreeImage_SetTagValue(tag.get(), val.c_str())) {
            return FreeImage_SetMetadata(model, dib, key, tag.get());
        }
    }
    return false;
}

#endif //__cplusplus



#endif // FREEIMAGEEXT_H

