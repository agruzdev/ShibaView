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

#include "ImageLoader.h"

#include <QImage>
#include <QFileInfo>
#include <QDebug>

#include "Image.h"

ImageLoader::QtMetaRegisterInvoker::QtMetaRegisterInvoker()
{
    qRegisterMetaType<ImagePtr>("ImagePtr");
    qRegisterMetaType<size_t>("size_t");
}

ImageLoader::QtMetaRegisterInvoker ImageLoader::msQtRegisterInvoker{};


ImageLoader::ImageLoader(const QString & name, size_t imgIdx, size_t imgCount)
    : QObject(nullptr)
    , mName(name), mImgIdx(imgIdx), mImgCount(imgCount)
{ }

ImageLoader::ImageLoader(const QString & name)
     : ImageLoader(name, 0, 0)
 { }

ImageLoader::~ImageLoader() = default;

void ImageLoader::onRun(const QString & path)
{
    bool success = false;
    QString error;

    try {
        auto pimage = QSharedPointer<Image>::create(mName, path);
        emit eventResult(std::move(pimage), mImgIdx, mImgCount);
        success = true;
    }
    catch(std::exception & e) {
        error = QString::fromUtf8(e.what());
        qWarning() << error;
    }
    catch(...) {
        error = QString("Unknown error!");
        qWarning() << error;
    }

    if(!success) {
        emit eventError(error);
    }

    deleteLater();
}

