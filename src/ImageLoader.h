/**
 * @file
 *
 * Copyright 2018-2026 Alexey Gruzdev
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

#ifndef IMAGELOADER_H
#define IMAGELOADER_H

#include <QObject>

#include "Image.h"


namespace fi
{
    class MessageView;
}


struct ImageLoadResult
{
    ImagePtr image;
    QStringList errors;
    size_t imgIdx;
    size_t imgCount;
};


class ImageLoader
    : public QObject
{
    Q_OBJECT

public:
    explicit
    ImageLoader(const QString & name);
    ImageLoader(const QString & name, size_t imgIdx, size_t imgCount);

    ~ImageLoader();

signals:
    void eventResult(ImageLoadResult result);

    void eventMessage(QDateTime time, QString what);
    void eventError(QString what);

public slots:
    void onRun(const QString & path);

private:
    void processMessageImpl(const fi::MessageView& msg);

    QString mName;
    size_t mImgIdx;
    size_t mImgCount;

    QStringList mLoadErrors;

    /**
     * Helper class for class registration
     */
    struct QtMetaRegisterInvoker
    {
        QtMetaRegisterInvoker();
    };
    static QtMetaRegisterInvoker msQtRegisterInvoker;
};

#endif // IMAGELOADER_H
