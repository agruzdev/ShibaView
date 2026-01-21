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

#include "Tooltip.h"

#include <QApplication>
#include <QColor>
#include <QScreen>
#include "TextWidget.h"

Tooltip::Tooltip()
    : mDefaultOffset(7, 20)
{
    mTextWidget = std::make_unique<TextWidget>(nullptr, Qt::black, 12);
    mTextWidget->setWindowFlags(Qt::ToolTip);
    mTextWidget->setBackgroundColor(QColor::fromRgb(255, 255, 225));
    mTextWidget->setBorderColor(Qt::black);
    mTextWidget->setPaddings(4, 2, 0, 0);
}

Tooltip::~Tooltip()
{
    mTextWidget->close();
}

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
    QRect screenGeometry = QRect(0, 0, 0, 0);
    if (const auto screen = QApplication::screenAt(position)) {
        screenGeometry = screen->geometry();
    }
    else if (const auto screen = QApplication::primaryScreen()) {
        screenGeometry = screen->geometry();
    }

    const QSize textSize = mTextWidget->size();

    QPoint tooltipPosition = position + mDefaultOffset;

    const int dx = tooltipPosition.x() + textSize.width() - screenGeometry.x() - screenGeometry.width();
    if (dx > 0) {
        tooltipPosition.setX(tooltipPosition.x() - dx);
    }
    const int dy = tooltipPosition.y() + textSize.height() - screenGeometry.y() - screenGeometry.height();
    if (dy > 0) {
        tooltipPosition.setY(tooltipPosition.y() - dy);
    }

    mTextWidget->move(tooltipPosition);
    if (mTextWidget->isVisible()) {
        mTextWidget->update();
    }
}
