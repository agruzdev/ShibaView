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

#include "Exif.h"
#include "FreeImageExt.h"

Exif Exif::load(FIBITMAP* dib)
{
    Exif exif{};
    for (auto model : { FIMD_COMMENTS, FIMD_EXIF_MAIN, FIMD_EXIF_EXIF, FIMD_EXIF_GPS, FIMD_EXIF_MAKERNOTE,
                    FIMD_EXIF_INTEROP, FIMD_IPTC, /*FIMD_XMP,*/ FIMD_GEOTIFF, FIMD_ANIMATION, FIMD_CUSTOM })
    {
        FITAG *tag = nullptr;
        std::unique_ptr<FIMETADATA, decltype(&::FreeImage_FindCloseMetadata)> mdhandle(FreeImage_FindFirstMetadata(model, dib, &tag), &::FreeImage_FindCloseMetadata);
        if (mdhandle) {
            do {
                QString key = QString::fromUtf8(FreeImage_GetTagKey(tag));
                QVariant value;
                switch(FreeImage_GetTagType(tag)) {
                case FIDT_BYTE:
                    value = QVariant::fromValue(FreeImageExt_GetTagValue<uint8_t>(tag));
                    break;
                case FIDT_SHORT:
                    value = QVariant::fromValue(FreeImageExt_GetTagValue<uint16_t>(tag));
                    break;
                case FIDT_LONG:
                case FIDT_IFD:
                    value = QVariant::fromValue(FreeImageExt_GetTagValue<uint32_t>(tag));
                    break;
                case FIDT_SBYTE:
                    value = QVariant::fromValue(FreeImageExt_GetTagValue<int8_t>(tag));
                    break;
                case FIDT_SSHORT:
                    value = QVariant::fromValue(FreeImageExt_GetTagValue<int16_t>(tag));
                    break;
                case FIDT_SLONG:
                    value = QVariant::fromValue(FreeImageExt_GetTagValue<int32_t>(tag));
                    break;
                case FIDT_FLOAT:
                    value = QVariant::fromValue(FreeImageExt_GetTagValue<float>(tag));
                    break;
                case FIDT_DOUBLE:
                    value = QVariant::fromValue(FreeImageExt_GetTagValue<double>(tag));
                    break;
                case FIDT_LONG8:
                case FIDT_IFD8:
                    value = QVariant::fromValue(FreeImageExt_GetTagValue<uint64_t>(tag));
                    break;
                case FIDT_SLONG8:
                    value = QVariant::fromValue(FreeImageExt_GetTagValue<int64_t>(tag));
                    break;
                case FIDT_RATIONAL:
                case FIDT_SRATIONAL:
                case FIDT_PALETTE:
                case FIDT_UNDEFINED:
                case FIDT_ASCII:
                default:
                    value = QString::fromUtf8(FreeImage_TagToString(model, tag));
                    break;
                }
                if (value.isValid()) {
                    exif.sections[model].emplace_back(std::move(key), std::move(value));
                }

            } while(FreeImage_FindNextMetadata(mdhandle.get(), &tag));
        }
    }
    if (FreeImage_GetThumbnail(dib)) {
        exif.sections[FIMD_CUSTOM].emplace_back("StoredThumbnail", "Yes");
    }
    return exif;
}
