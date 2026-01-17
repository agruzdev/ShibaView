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

#ifndef VIEWERAPPLICATION_H
#define VIEWERAPPLICATION_H

#include <memory>

#include <QApplication>
#include <QDir>
#include <QThread>
#include <QStringList>
#include <QFileSystemWatcher>

#include <CanvasWidget.h>

struct FIMESSAGE;
class LoggerWidget;

class ViewerApplication
    : public QObject
{
    Q_OBJECT

public:
    ViewerApplication(std::chrono::steady_clock::time_point t);
    ~ViewerApplication();

    ViewerApplication(const ViewerApplication&) = delete;
    ViewerApplication(ViewerApplication&&) = delete;

    ViewerApplication& operator=(const ViewerApplication&) = delete;
    ViewerApplication& operator=(ViewerApplication&&) = delete;

    void open(const QString & path);

signals:
    void eventLoadImage(const QString & path);
    void eventCancelTransition();

    void eventImageDirScanned(size_t imgIdx, size_t totalCount);

    void eventMessage(QDateTime time, QString what);

public slots:
    void onNextImage();
    void onPrevImage();
    void onFirstImage();
    void onLastImage();
    void onReloadImage();
    void onOpenImage();
    void onToggleLog();

    void onError(const QString& what);

    void onDirectoryChanged(const QString &path);

private:
    void loadImageAsync(const QString & path, size_t imgIdx, size_t totalCount);
    void scanDirectory();

    void processMessageImpl(const FIMESSAGE* msg);

    std::unique_ptr<LoggerWidget> mLoggerWidget = nullptr;
    std::unique_ptr<CanvasWidget> mCanvasWidget = nullptr;
    std::unique_ptr<QThread> mBackgroundThread = nullptr;

    QString mOpenedName;
    QDir mDirectory;
    QFileSystemWatcher mDirWatcher;

    QStringList mFilesInDirectory;
    QStringList::const_iterator mCurrentFile;
    size_t mCurrentIdx = 0;
    uint32_t mLogProcId = 0;
};

#endif // VIEWERAPPLICATION_H
