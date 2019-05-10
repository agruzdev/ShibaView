/**
 * @file
 *
 * Copyright 2018-2019 Alexey Gruzdev
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

#ifndef IMAGEINFO_H
#define IMAGEINFO_H

#include <QDateTime>
#include <QSize>
#include <QString>

struct ImageSize
{
    uint32_t width  = 0;
    uint32_t height = 0;
};

struct ImageInfo
{
    QString path;
    QString name;
    size_t bytes = 0;
    //QString format;
    QDateTime modified;
    ImageSize dims;
};

#endif // IMAGEINFO_H
