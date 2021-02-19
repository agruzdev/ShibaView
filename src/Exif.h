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

#ifndef EXIF_H
#define EXIF_H

#include "FreeImage.h"
#include <QString>
#include <QVariant>

struct Exif
{
    std::array<std::vector<std::tuple<QString, QVariant>>, FIMD_EXIF_RAW + 1> sections;

    static
    Exif load(FIBITMAP* dib);
};


#endif // EXIF_H