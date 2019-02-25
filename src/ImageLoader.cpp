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

#include "ImageLoader.h"

#include <QImage>
#include <QFileInfo>
#include <QDebug>

#include "Image.h"

ImageLoader::ImageLoader(const QString & name)
    : QObject(nullptr)
    , mName(name)
{
    qRegisterMetaType<ImagePtr>("ImagePtr");
}

ImageLoader::~ImageLoader() = default;

void ImageLoader::onRun(const QString & path)
{
    bool success = false;
    try {
        auto pimage = QSharedPointer<Image>::create(mName, path);
        emit eventResult(pimage);
        success = true;
    }
    catch(std::exception & e) {
        qWarning() << QString(e.what());
    }
    catch(...) {
        qWarning() << QString("Unknown error!");
    }

    if(!success) {
        ImageInfo info;
        info.path = path;
        emit eventResult(nullptr);
    }
    deleteLater();
}

