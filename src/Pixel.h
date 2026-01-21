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

#ifndef PIXEL_H
#define PIXEL_H

#include "FreeImage.h"
#include <QString>

struct Pixel
{
    uint32_t y = 0;
    uint32_t x = 0;
    QString repr;

    static
    bool getBitmapPixel(FIBITMAP* src, uint32_t y, uint32_t x, Pixel* pixel);
};


template <typename Ty_>
inline
QString numberToQString(Ty_ v)
{
    return QString::number(v);
}

template <>
inline
QString numberToQString<float>(float v)
{
    return QString::number(v, 'g', 4);
}

template <>
inline
QString numberToQString<double>(double v)
{
    return QString::number(v, 'g', 6);
}


template <typename PTy_>
inline
QString pixelToString4(const uint8_t* raw)
{
    const auto p = static_cast<const PTy_*>(static_cast<const void*>(raw));
    return QString("%1, %2, %3, %4").arg(numberToQString(p->red)).arg(numberToQString(p->green)).arg(numberToQString(p->blue)).arg(numberToQString(p->alpha));
}

template <typename PTy_>
inline
QString pixelToString3(const uint8_t* raw)
{
    const auto p = static_cast<const PTy_*>(static_cast<const void*>(raw));
    return QString("%1, %2, %3").arg(numberToQString(p->red)).arg(numberToQString(p->green)).arg(numberToQString(p->blue));
}

template <typename PTy_>
inline
QString pixelToString2(const uint8_t* raw)
{
    const auto p = static_cast<const PTy_*>(static_cast<const void*>(raw));
    return QString("%1, %2").arg(numberToQString(p->r)).arg(numberToQString(p->i));
}

template <typename PTy_>
inline
QString pixelToString1(const uint8_t* raw)
{
    const auto p = static_cast<const PTy_*>(static_cast<const void*>(raw));
    return numberToQString(*p);
}

#endif // PIXEL_H
