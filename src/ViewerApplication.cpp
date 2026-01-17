/**
 * @file
 *
 * Copyright 2018-2023 Alexey Gruzdev
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

#include "ViewerApplication.h"

#include <iostream>

#include <QCollator>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QSettings>

#include "Global.h"
#include "ImageLoader.h"
#include "PluginManager.h"
#include "LoggerWidget.h"
#include "FreeImageExt.h"



ViewerApplication::ViewerApplication(std::chrono::steady_clock::time_point t)
{
    mLogProcId = FreeImage_AddProcessMessageFunction(this, [](void* thiz, const FIMESSAGE* msg) {
        static_cast<ViewerApplication*>(thiz)->processMessageImpl(msg);
    });

    PluginManager::getInstance().init(PluginUsage::eViewer);

    mLoggerWidget = std::make_unique<LoggerWidget>();
    mLoggerWidget->hide();
    connect(this, &ViewerApplication::eventMessage, mLoggerWidget.get(), &LoggerWidget::onMessage, Qt::QueuedConnection);

    mCanvasWidget = std::make_unique<CanvasWidget>(t);

    connect(mCanvasWidget.get(), &CanvasWidget::eventNextImage,   this, &ViewerApplication::onNextImage,   Qt::QueuedConnection);
    connect(mCanvasWidget.get(), &CanvasWidget::eventPrevImage,   this, &ViewerApplication::onPrevImage,   Qt::QueuedConnection);
    connect(mCanvasWidget.get(), &CanvasWidget::eventFirstImage,  this, &ViewerApplication::onFirstImage,  Qt::QueuedConnection);
    connect(mCanvasWidget.get(), &CanvasWidget::eventLastImage,   this, &ViewerApplication::onLastImage,   Qt::QueuedConnection);
    connect(mCanvasWidget.get(), &CanvasWidget::eventReloadImage, this, &ViewerApplication::onReloadImage, Qt::QueuedConnection);
    connect(mCanvasWidget.get(), &CanvasWidget::eventOpenImage,   this, &ViewerApplication::onOpenImage,   Qt::QueuedConnection);
    connect(mCanvasWidget.get(), &CanvasWidget::eventToggleLog,   this, &ViewerApplication::onToggleLog,   Qt::QueuedConnection);
    connect(this, &ViewerApplication::eventCancelTransition, mCanvasWidget.get(), &CanvasWidget::onTransitionCanceled, Qt::QueuedConnection);
    connect(this, &ViewerApplication::eventImageDirScanned,  mCanvasWidget.get(), &CanvasWidget::onImageDirScanned,    Qt::QueuedConnection);

    mBackgroundThread.reset(new QThread);
    mBackgroundThread->start();

    connect(&mDirWatcher, &QFileSystemWatcher::directoryChanged, this, &ViewerApplication::onDirectoryChanged);
}

ViewerApplication::~ViewerApplication()
{
    FreeImage_RemoveProcessMessageFunction(mLogProcId);

    mBackgroundThread->quit();
    mBackgroundThread->wait();
}

void ViewerApplication::loadImageAsync(const QString &path, size_t imgIdx, size_t totalCount)
{
    auto loader = std::make_unique<ImageLoader>(QFileInfo(path).fileName(), imgIdx, totalCount);
    connect(this, &ViewerApplication::eventLoadImage, loader.get(), &ImageLoader::onRun, Qt::QueuedConnection);
    connect(loader.get(), &ImageLoader::eventResult,  mCanvasWidget.get(), &CanvasWidget::onImageReady, Qt::QueuedConnection);
    connect(loader.get(), &ImageLoader::eventMessage, mLoggerWidget.get(), &LoggerWidget::onMessage,    Qt::QueuedConnection);
    connect(loader.get(), &ImageLoader::eventError,   this, &ViewerApplication::onError, Qt::QueuedConnection);
    loader->moveToThread(mBackgroundThread.get());

    emit eventMessage(QDateTime::currentDateTime(), path);
    emit eventLoadImage(path);

    disconnect(this, &ViewerApplication::eventLoadImage, loader.get(), &ImageLoader::onRun);
    loader.release();
}

void ViewerApplication::scanDirectory()
{
    if(!mDirectory.exists()) {
        mFilesInDirectory.clear();
        mCurrentFile  = mFilesInDirectory.cend();
        mCurrentIdx = 0;
    }
    else {
        QCollator collator;
        collator.setNumericMode(true);
        mFilesInDirectory = mDirectory.entryList(Global::getSupportedExtensionFilters(), QDir::Files);
        std::sort(mFilesInDirectory.begin(), mFilesInDirectory.end(), collator);

        mCurrentIdx = 0;
        for (mCurrentFile = mFilesInDirectory.cbegin(); mCurrentFile != mFilesInDirectory.cend(); ++mCurrentFile, ++mCurrentIdx) {
            if (*mCurrentFile == mOpenedName) {
                break;
            }
        }

        if (mCurrentFile != mFilesInDirectory.cend()) {
            emit eventImageDirScanned(mCurrentIdx, mFilesInDirectory.size());
        }
        else {
            emit eventImageDirScanned(0, 0);
        }
    }
}

void ViewerApplication::onDirectoryChanged(const QString & /*path*/)
{
    scanDirectory();
}

void ViewerApplication::open(const QString & path)
{
    QFileInfo finfo(path);
    mOpenedName = finfo.fileName();
    loadImageAsync(finfo.absoluteFilePath(), 0, 0);

    mDirectory = finfo.dir();
    mDirWatcher.addPath(mDirectory.absolutePath());
    scanDirectory();
}

void ViewerApplication::onError(const QString& what)
{
    qWarning() << what;
    if (mCanvasWidget) {
        mCanvasWidget->close();
    }
    if (mLoggerWidget) {
        mLoggerWidget->close();
    }
    QApplication::exit(-1);
}

void ViewerApplication::onNextImage()
{
    if(!mFilesInDirectory.empty()) {
        if(mCurrentFile != mFilesInDirectory.cend()) {
            ++mCurrentFile;
            ++mCurrentIdx;
            if(mCurrentFile == mFilesInDirectory.cend()) {
                mCurrentFile = mFilesInDirectory.cbegin();
                mCurrentIdx  = 0;
            }
        }
        else {
            mCurrentFile = mFilesInDirectory.cbegin();
            mCurrentIdx  = 0;
        }
        mOpenedName = *mCurrentFile;
        loadImageAsync(mDirectory.absoluteFilePath(mOpenedName), mCurrentIdx, mFilesInDirectory.size());
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
                mCurrentIdx = mFilesInDirectory.size();
            }
            --mCurrentFile;
            --mCurrentIdx;
        }
        else {
            mCurrentFile = std::prev(mFilesInDirectory.cend());
            mCurrentIdx = mFilesInDirectory.size() - 1;
        }
        mOpenedName = *mCurrentFile;
        loadImageAsync(mDirectory.absoluteFilePath(mOpenedName), mCurrentIdx, mFilesInDirectory.size());
    }
    else {
        emit eventCancelTransition();
    }
}

void ViewerApplication::onFirstImage()
{
    if(!mFilesInDirectory.empty()) {
        mCurrentFile = mFilesInDirectory.cbegin();
        mCurrentIdx  = 0;
        mOpenedName = *mCurrentFile;
        loadImageAsync(mDirectory.absoluteFilePath(mOpenedName), mCurrentIdx, mFilesInDirectory.size());
    }
    else {
        emit eventCancelTransition();
    }
}

void ViewerApplication::onLastImage()
{
    if(!mFilesInDirectory.empty()) {
        mCurrentFile = std::prev(mFilesInDirectory.cend());
        mCurrentIdx = mFilesInDirectory.size() - 1;
        mOpenedName = *mCurrentFile;
        loadImageAsync(mDirectory.absoluteFilePath(mOpenedName), mCurrentIdx, mFilesInDirectory.size());
    }
    else {
        emit eventCancelTransition();
    }
}

void ViewerApplication::onReloadImage()
{
    if (mDirectory.exists() && !mOpenedName.isEmpty()) {
        loadImageAsync(mDirectory.absoluteFilePath(mOpenedName), mCurrentIdx, mFilesInDirectory.size());
    }
    else {
        emit eventCancelTransition();
    }
}

void ViewerApplication::onOpenImage()
{
    QString input = QFileDialog::getOpenFileName(nullptr, "Open File", mDirectory.absolutePath(), Global::getSupportedExtensionsFilterString() + ";;All files (*.*)");
    if (!input.isEmpty()) {
        open(input);
    }
    else {
        emit eventCancelTransition();
    }
}

void ViewerApplication::onToggleLog()
{
    if (mLoggerWidget) {
        if (mLoggerWidget->isVisible()) {
            mLoggerWidget->hide();
        }
        else {
            mLoggerWidget->show();
        }
    }
}

void ViewerApplication::processMessageImpl(const FIMESSAGE* msg)
{
    const char* what = FreeImage_GetMessageString(msg);
    if (!what) {
        return;
    }

    emit eventMessage(QDateTime::currentDateTime(), QString::fromUtf8(what));
}
