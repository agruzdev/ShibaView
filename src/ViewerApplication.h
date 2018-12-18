/**
* ShibaView
*
* The MIT License (MIT)
* Copyright (c) 2018 Alexey Gruzdev
*/

#ifndef VIEWERAPPLICATION_H
#define VIEWERAPPLICATION_H

#include <memory>

#include <QApplication>
#include <QDir>
#include <QThread>
#include <QStringList>

#include <CanvasWidget.h>

class ViewerApplication
    : public QObject
{
    Q_OBJECT

public:
    ViewerApplication(std::chrono::steady_clock::time_point t);
    ~ViewerApplication();

    void loadImageAsync(const QString & path);

    void open(const QString & path);

    ViewerApplication(const ViewerApplication&) = delete;
    ViewerApplication(ViewerApplication&&) = delete;

    ViewerApplication& operator=(const ViewerApplication&) = delete;
    ViewerApplication& operator=(ViewerApplication&&) = delete;

signals:
    void eventLoadImage(const QString & path);
    void eventCancelTransition();

public slots:
    void onNextImage();
    void onPrevImage();
    void onFirstImage();
    void onLastImage();

    void onError(const QString & what);

private:
    std::unique_ptr<CanvasWidget> mCanvasWidget = nullptr;
    std::unique_ptr<QThread> mBackgroundThread = nullptr;

    QDir mDirectory;
    QStringList mFilesInDirectory;
    QStringList::const_iterator mCurrentFile;
};

#endif // VIEWERAPPLICATION_H
