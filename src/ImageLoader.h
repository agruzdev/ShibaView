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

struct ImageInfo
{
    QString name;
    QSize dims;
    size_t bytes;
    QDateTime modified;

    QString toString()
    {
        return
            "File name: " + name + "\n" +
            "File size: " + QString::number(bytes / 1024.0f) + "KB\n" +
            "Last modified: " + modified.toString("yyyy/MM/dd hh:mm:ss") + "\n" +
            "Resolution: " + QString::number(dims.width()) + "x" + QString::number(dims.height());
    }
};

Q_DECLARE_METATYPE(ImageInfo)

class ImageLoader
    : public QObject
{
    Q_OBJECT

public:
    ImageLoader();
    ~ImageLoader();

signals:
    void eventResult(QPixmap p, ImageInfo i);

public slots:
    void onRun(const QString & path);

private:
};

#endif // IMAGELOADER_H
