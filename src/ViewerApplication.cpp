/**
* ShibaView
*
* The MIT License (MIT)
* Copyright (c) 2018 Alexey Gruzdev
*/

#include "ViewerApplication.h"
#include "ImageLoader.h"

#include <iostream>

ViewerApplication::ViewerApplication(std::chrono::steady_clock::time_point t)
{
    mCanvasWidget.reset(new CanvasWidget(t));
    mCanvasWidget->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

    mBackgroundThread.reset(new QThread);
    mBackgroundThread->start();
}

ViewerApplication::~ViewerApplication()
{
    mBackgroundThread->quit();
    mBackgroundThread->wait();
}

void ViewerApplication::loadImageAsync(const QString &path)
{
    ImageLoader* loader = new ImageLoader;
    connect(this, &ViewerApplication::eventLoadImage, loader, &ImageLoader::onRun);
    connect(loader, &ImageLoader::eventResult, mCanvasWidget.get(), &CanvasWidget::onImageReady);
    loader->moveToThread(mBackgroundThread.get());
    emit eventLoadImage(path);
    disconnect(this, &ViewerApplication::eventLoadImage, loader, &ImageLoader::onRun);
}
