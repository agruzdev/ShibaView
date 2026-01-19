/**
 * @file
 *
 * Copyright 2018-2023 Alexey Gruzdev
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

#ifndef LOGGER_WIDGET_H
#define LOGGER_WIDGET_H

#include <QDateTime>
#include <QWidget>


class QScrollArea;
class TextWidget;

class LoggerWidget final
    : public QWidget
{
    Q_OBJECT

public:
    LoggerWidget(QWidget* parent = nullptr);

    ~LoggerWidget() override;

    LoggerWidget(const LoggerWidget&) = delete;

    LoggerWidget(LoggerWidget&&) = delete;

    LoggerWidget& operator=(const LoggerWidget&) = delete;

    LoggerWidget& operator=(LoggerWidget&&) = delete;

public slots:
    void onMessage(const QDateTime& time, const QString& what);

protected:
    void paintEvent(QPaintEvent* event) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent* event) Q_DECL_OVERRIDE;

private:
    TextWidget* mText{ nullptr };
    QScrollArea* mScrollArea{ nullptr };
    QStringList mRecords;
    bool mTextIsValid{ false };
};

#endif // LOGGER_WIDGET_H
