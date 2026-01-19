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

#include "LoggerWidget.h"

#include <QKeyEvent>
#include <QScrollArea>
#include <QSettings>
#include <QVBoxLayout>

#include "Global.h"
#include "TextWidget.h"


namespace
{
    constexpr size_t kMaxRecondsNumber = 256;

    constexpr int32_t kMinimumHeight  = 200;
    constexpr int32_t kMinimumWidth   = 300;
    constexpr int32_t kMinimumPadding = 10;

    constexpr int32_t kDefaultHeight = 600;
    constexpr int32_t kDefaultWidth  = 500;

    const QString kSettingsSize = "log/size";

} // namespace



LoggerWidget::LoggerWidget(QWidget* parent)
    : QWidget(parent)
{
    setWindowTitle(Global::makeTitle("Log"));

    auto layout = new QVBoxLayout(this);

    mScrollArea = new QScrollArea(this);
    mScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mScrollArea->setWidgetResizable(false);
    mScrollArea->setStyleSheet("QScrollArea { border: none; }");

    mText = new TextWidget(nullptr, {}, 11, 0.8);
    mText->setPaddings(4, 0, 4, 0);
    mText->setColumnSeperator('|');
    mText->appendColumnOffset(80);

    mScrollArea->setWidget(mText);
    layout->addWidget(mScrollArea);

    mScrollArea->setMinimumSize(kMinimumWidth, kMinimumHeight);
    mScrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QSettings settings;
    resize(settings.value(kSettingsSize, QSize(kDefaultWidth, kDefaultHeight)).toSize());
}


LoggerWidget::~LoggerWidget() = default;


void LoggerWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);

    if (!mTextIsValid) {
        mText->setText(mRecords);
        mTextIsValid = true;

        mScrollArea->ensureVisible(0, std::numeric_limits<int>::max());
    }
}


void LoggerWidget::onMessage(const QDateTime& time, const QString& what)
{
    while (mRecords.size() >= kMaxRecondsNumber) {
        mRecords.pop_front();
    }
    mRecords.push_back(time.toString("hh:mm:ss") + " | " + what);
    mTextIsValid = false;
    update();
}


void LoggerWidget::keyPressEvent(QKeyEvent* event)
{
    QWidget::keyPressEvent(event);
    if (event->key() == Qt::Key_Escape) {
        close();
    }
}
