/**
* ShibaView
*
* The MIT License (MIT)
* Copyright (c) 2018 Alexey Gruzdev
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

