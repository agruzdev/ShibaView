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

#include "ImageLoader.h"

#include <QImage>
#include <QFileInfo>
#include <QDebug>

#include "FreeImageExt.h"
#include "Image.h"

namespace {


} // namespace


ImageLoader::QtMetaRegisterInvoker::QtMetaRegisterInvoker()
{
    qRegisterMetaType<ImagePtr>("ImagePtr");
    qRegisterMetaType<size_t>("size_t");
    qRegisterMetaType<ImageLoadResult>("ImageLoadResult");
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
    mLoadErrors.clear();
    bool success = false;
    QString error;

    try {
        fi::MessageProcessFunctionGuard msgProc([this](const fi::MessageView& msg) { processMessageImpl(msg); });

        ImageLoadResult result{};
        result.image = QSharedPointer<Image>::create(mName, path);
        result.imgCount = mImgCount;
        result.imgIdx = mImgIdx;
        result.errors.swap(mLoadErrors);
        emit eventResult(std::move(result));
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

    if (!success) {
        emit eventError(std::move(error));
    }

    deleteLater();
}


void ImageLoader::processMessageImpl(const fi::MessageView& msg)
{
    const char* what = msg.GetCString();
    if (!what) {
        return;
    }

    auto qwhat = QString::fromUtf8(what);

    qWarning() << qwhat;
    mLoadErrors.push_back(qwhat);

    emit eventMessage(QDateTime::currentDateTime(), std::move(qwhat));
}

