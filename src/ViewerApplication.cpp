/**
* ShibaView
*
* The MIT License (MIT)
* Copyright (c) 2018 Alexey Gruzdev
*/

#include "ViewerApplication.h"

#include <iostream>

#include <QDir>
#include <QFileInfo>

#include "ImageLoader.h"

namespace
{
    const char* kSupportedExtensions[] = {
        "*.png",
        "*.jpg", "*.jpeg",
        "*.tga",
        "*.tif", "*.tiff",
        "*.bmp",
        "*.gif",
        "*.pbm", "*.pgm", "*.ppm", "*.pnm", "*.pfm", "*.pam",
        "*.hdr"
    };
}

ViewerApplication::ViewerApplication(std::chrono::steady_clock::time_point t)
{
    mCanvasWidget.reset(new CanvasWidget(this, t));
    mCanvasWidget->setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::MSWindowsOwnDC);

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
    connect(loader, &ImageLoader::eventError, this, &ViewerApplication::onError);
    loader->moveToThread(mBackgroundThread.get());
    emit eventLoadImage(path);
    disconnect(this, &ViewerApplication::eventLoadImage, loader, &ImageLoader::onRun);
}

void ViewerApplication::open(const QString & path)
{
    QFileInfo finfo(path);
    if(!finfo.exists() | finfo.isDir()) {
        throw std::runtime_error("Input doesn't exist");
    }
    mDirectory = finfo.dir();
    QStringList extensions;
    for(const auto & ext : kSupportedExtensions) {
        extensions << ext;
    }
    mFilesInDirectory = mDirectory.entryList(extensions);

    mCurrentFile = std::find(mFilesInDirectory.cbegin(), mFilesInDirectory.cend(), finfo.fileName());
}

void ViewerApplication::onError(const QString & what)
{
    qWarning() << what;
    if(mCanvasWidget) {
        mCanvasWidget->close();
    }
    QApplication::exit(-1);
}

void ViewerApplication::onNextImage()
{
    if(mCurrentFile != mFilesInDirectory.cend() && !mFilesInDirectory.empty()) {
        ++mCurrentFile;
        if(mCurrentFile == mFilesInDirectory.cend()) {
            mCurrentFile = mFilesInDirectory.cbegin();
        }
        loadImageAsync(mDirectory.absoluteFilePath(*mCurrentFile));
    }
}

void ViewerApplication::onPrevImage()
{
    if(mCurrentFile != mFilesInDirectory.cend() && !mFilesInDirectory.empty()) {
        if(mCurrentFile == mFilesInDirectory.begin()) {
            mCurrentFile = mFilesInDirectory.cend();
        }
        --mCurrentFile;
        loadImageAsync(mDirectory.absoluteFilePath(*mCurrentFile));
    }
}
