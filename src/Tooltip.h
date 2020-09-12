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

#ifndef TOOLTIP_H
#define TOOLTIP_H

#include <memory>

#include <QPoint>
#include <QString>
#include <QStringList>

class TextWidget;

class Tooltip
{
public:
    Tooltip();

    Tooltip(const Tooltip&) = delete;

    Tooltip(Tooltip&&) = delete;

    ~Tooltip();

    Tooltip& operator=(const Tooltip&) = delete;

    Tooltip& operator=(Tooltip&&) = delete;

    void hide();

    void show();

    void setText(const QVector<QString>& lines);

    void move(const QPoint& position);

private:
    std::unique_ptr<TextWidget> mTextWidget;
    QPoint mDefaultOffset;
};

#endif // TOOLTIP_H
