/**
 * @file
 *
 * Copyright 2018-2020 Alexey Gruzdev
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

#ifndef EXIF_WIDGET_H
#define EXIF_WIDGET_H

#include <QWidget>
#include "FreeImage.h"

class TextWidget;

class ExifWidget final
    : public QWidget
{
    Q_OBJECT

public:
    static
    ExifWidget& getInstance();

    ExifWidget(const ExifWidget&) = delete;

    ExifWidget(ExifWidget&&) = delete;

    ExifWidget& operator=(const ExifWidget&) = delete;

    ExifWidget& operator=(ExifWidget&&) = delete;

    void activate();

    void readExifFrom(FIBITMAP* bmp);

    bool isActive()
    {
        return mActive;
    }

public:
    ExifWidget();

    ~ExifWidget() override;

    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent * event) Q_DECL_OVERRIDE;
    void closeEvent(QCloseEvent* event) Q_DECL_OVERRIDE;

    bool mActive = false;
    TextWidget* mText;
};

#endif // EXIF_WIDGET_H
