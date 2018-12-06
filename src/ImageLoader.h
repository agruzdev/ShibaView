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

class ImageLoader
    : public QObject
{
    Q_OBJECT
public:
    ImageLoader();
    ~ImageLoader();

signals:
    void eventResult(QPixmap p);

public slots:
    void onRun(const QString & path);
};

#endif // IMAGELOADER_H
