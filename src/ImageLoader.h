/**
 * @file
 *
 * Copyright 2018-2019 Alexey Gruzdev
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
