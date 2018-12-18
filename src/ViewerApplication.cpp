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
#include <QCollator>

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
    mCanvasWidget.reset(new CanvasWidget(t));
    mCanvasWidget->setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::MSWindowsOwnDC);

    connect(mCanvasWidget.get(), &CanvasWidget::eventNextImage,  this, &ViewerApplication::onNextImage,  Qt::QueuedConnection);
    connect(mCanvasWidget.get(), &CanvasWidget::eventPrevImage,  this, &ViewerApplication::onPrevImage,  Qt::QueuedConnection);
    connect(mCanvasWidget.get(), &CanvasWidget::eventFirstImage, this, &ViewerApplication::onFirstImage, Qt::QueuedConnection);
    connect(mCanvasWidget.get(), &CanvasWidget::eventLastImage,  this, &ViewerApplication::onLastImage,  Qt::QueuedConnection);
    connect(this, &ViewerApplication::eventCancelTransition, mCanvasWidget.get(), &CanvasWidget::onTransitionCanceled, Qt::QueuedConnection);

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
    QString name = QFileInfo(path).fileName();
    if (mCurrentFile != mFilesInDirectory.cend()) {
        const auto index = std::distance(mFilesInDirectory.cbegin(), mCurrentFile);
        name.append(" [" + QString::number(index + 1) + '/' + QString::number(mFilesInDirectory.size()) + "]");
    }
    ImageLoader* loader = new ImageLoader(name);
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
    mDirectory = finfo.dir();
    if(!mDirectory.exists()) {
        mFilesInDirectory.clear();
        mCurrentFile  = mFilesInDirectory.cend();
    }
    else {
        QStringList extensions;
        for(const auto & ext : kSupportedExtensions) {
            extensions << ext;
        }
        QCollator collator;
        collator.setNumericMode(true);
        mFilesInDirectory = mDirectory.entryList(extensions);
        std::sort(mFilesInDirectory.begin(), mFilesInDirectory.end(), collator);
        mCurrentFile = std::find(mFilesInDirectory.cbegin(), mFilesInDirectory.cend(), finfo.fileName());
    }
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
    if(!mFilesInDirectory.empty()) {
        if(mCurrentFile != mFilesInDirectory.cend()) {
            ++mCurrentFile;
            if(mCurrentFile == mFilesInDirectory.cend()) {
                mCurrentFile = mFilesInDirectory.cbegin();
            }
        }
        else {
            mCurrentFile = mFilesInDirectory.cbegin();
        }
        loadImageAsync(mDirectory.absoluteFilePath(*mCurrentFile));
    }
    else {
        emit eventCancelTransition();
    }
}

void ViewerApplication::onPrevImage()
{
    if(!mFilesInDirectory.empty()) {
        if(mCurrentFile != mFilesInDirectory.cend()) {
            if(mCurrentFile == mFilesInDirectory.begin()) {
                mCurrentFile = mFilesInDirectory.cend();
            }
            --mCurrentFile;
        }
        else {
            mCurrentFile = std::prev(mFilesInDirectory.cend());
        }
        loadImageAsync(mDirectory.absoluteFilePath(*mCurrentFile));
    }
    else {
        emit eventCancelTransition();
    }
}

void ViewerApplication::onFirstImage()
{
    if(!mFilesInDirectory.empty()) {
        mCurrentFile = mFilesInDirectory.cbegin();
        loadImageAsync(mDirectory.absoluteFilePath(*mCurrentFile));
    }
    else {
        emit eventCancelTransition();
    }
}

void ViewerApplication::onLastImage()
{
    if(!mFilesInDirectory.empty()) {
        mCurrentFile = std::prev(mFilesInDirectory.cend());
        loadImageAsync(mDirectory.absoluteFilePath(*mCurrentFile));
    }
    else {
        emit eventCancelTransition();
    }
}
