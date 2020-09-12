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

#include "Tooltip.h"

#include <QApplication>
#include <QColor>
#include <QDesktopWidget>
#include <QScreen>
#include "TextWidget.h"

Tooltip::Tooltip()
    : mDefaultOffset(7, 20)
{
    mTextWidget = std::make_unique<TextWidget>(nullptr, Qt::black, 12);
    mTextWidget->setWindowFlags(Qt::ToolTip);
    mTextWidget->setBackgroundColor(QColor::fromRgb(255, 255, 225));
    mTextWidget->setBorderColor(Qt::black);
    mTextWidget->setPaddings(4, 2, 0, -4);
}

Tooltip::~Tooltip() = default;

void Tooltip::show()
{
    mTextWidget->setColor(Qt::black);
    mTextWidget->show();
    mTextWidget->update();
}

void Tooltip::hide()
{
    mTextWidget->hide();
    mTextWidget->setColor(Qt::transparent); // ToDo (a.gruzdev): Workaround to hide previous content
    mTextWidget->move(QPoint(0, 0));
}

void Tooltip::setText(const QVector<QString>& lines)
{
    mTextWidget->setText(lines);
}

void Tooltip::move(const QPoint& position)
{
    QRect screenGeometry;
    if (const auto screen = QApplication::screenAt(position)) {
        screenGeometry = screen->geometry();
    }
    else {
        screenGeometry = QApplication::desktop()->screenGeometry();
    }

    const QSize textSize = mTextWidget->size();

    QPoint adjustedPosition = position + mDefaultOffset;

    const int dx = adjustedPosition.x() + textSize.width() - screenGeometry.width();
    if (dx > 0) {
        adjustedPosition.setX(adjustedPosition.x() - dx);
    }
    const int dy = adjustedPosition.y() + textSize.height() - screenGeometry.height();
    if (dy > 0) {
        adjustedPosition.setY(adjustedPosition.y() - dy);
    }

    mTextWidget->move(adjustedPosition);
    if (mTextWidget->isVisible()) {
        mTextWidget->update();
    }
}
