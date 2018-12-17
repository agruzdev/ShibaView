#ifndef IMAGEINFO_H
#define IMAGEINFO_H

#include <QDateTime>
#include <QObject>
#include <QSize>
#include <QString>
#include <QVector>


struct ImageInfo
{
    QString path;
    QString name;
    size_t bytes = 0;
    QString format;
    QDateTime modified;
    QSize dims;

    QVector<QString> toLines() const
    {
        QVector<QString> res;
        res.push_back("File name: " + name);
        res.push_back("File size: " + QString::number(bytes / 1024.0f, 'f', 1) + "KB");
        res.push_back("Format: " + format);
        res.push_back("Last modified: " + modified.toString("yyyy/MM/dd hh:mm:ss"));
        res.push_back("Resolution: " + QString::number(dims.width()) + "x" + QString::number(dims.height()));
        res.push_back("");
        return res;
    }

    static
    int linesNumber()
    {
        return 6;
    }
};

Q_DECLARE_METATYPE(ImageInfo)



#endif // IMAGEINFO_H
