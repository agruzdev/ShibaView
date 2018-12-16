/**
* ShibaView
*
* The MIT License (MIT)
* Copyright (c) 2018 Alexey Gruzdev
*/

#ifndef IMAGELOADER_H
#define IMAGELOADER_H

#include <QObject>
#include <QPixmap>
#include <QDateTime>

#include "ImageInfo.h"

class ImageLoader
    : public QObject
{
    Q_OBJECT

public:
    ImageLoader();
    ~ImageLoader();

signals:
    void eventResult(QPixmap p, ImageInfo i);

    void eventError(QString what);

public slots:
    void onRun(const QString & path);

private:
};

#endif // IMAGELOADER_H
