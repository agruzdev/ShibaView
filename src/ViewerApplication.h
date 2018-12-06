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
#include <QThread>

#include <CanvasWidget.h>

class ViewerApplication
    : public QObject
{
    Q_OBJECT

public:
    ViewerApplication(std::chrono::steady_clock::time_point t);
    ~ViewerApplication();

    void loadImageAsync(const QString & path);

    ViewerApplication(const ViewerApplication&) = delete;
    ViewerApplication(ViewerApplication&&) = delete;

    ViewerApplication& operator=(const ViewerApplication&) = delete;
    ViewerApplication& operator=(ViewerApplication&&) = delete;

signals:
    void eventLoadImage(const QString & path);

private:
    std::unique_ptr<CanvasWidget> mCanvasWidget = nullptr;
    std::unique_ptr<QThread> mBackgroundThread = nullptr;
};

#endif // VIEWERAPPLICATION_H
