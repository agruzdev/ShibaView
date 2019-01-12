/**
* ShibaView
*
* The MIT License (MIT)
* Copyright (c) 2018 Alexey Gruzdev
*/

#ifndef IMAGELOADER_H
#define IMAGELOADER_H

#include <QObject>

#include "Image.h"

class ImageLoader
    : public QObject
{
    Q_OBJECT

public:
    ImageLoader(const QString & name);
    ~ImageLoader();

signals:
    void eventResult(ImagePtr image);

    void eventError(QString what);

public slots:
    void onRun(const QString & path);

private:
    QString mName;
};

#endif // IMAGELOADER_H
