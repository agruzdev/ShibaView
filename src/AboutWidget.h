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

#ifndef ABOUT_WIDGET_H
#define ABOUT_WIDGET_H

#include <QWidget>

class TextWidget;

class AboutWidget final
    : public QWidget
{
    Q_OBJECT

public:
    static
    void showInstance();

private:
    AboutWidget();

    AboutWidget(const AboutWidget&) = delete;

    AboutWidget(AboutWidget&&) = delete;

    ~AboutWidget() override;

    AboutWidget& operator=(const AboutWidget&) = delete;

    AboutWidget& operator=(AboutWidget&&) = delete;
};

#endif // ABOUT_WIDGET_H
