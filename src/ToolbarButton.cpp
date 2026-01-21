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

#include "ToolbarButton.h"
#include <QMouseEvent>
#include <QPainter>
#include "TextWidget.h"


#define UTF8_CROSS_SYMBOL "\xE2\x9C\x95"

ToolbarButton::ToolbarButton(QWidget *parent, QSize size)
    : QPushButton(parent)
    , mSize(size)
{
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    resize(mSize);

    mText = new TextWidget(this, {}, 18.0, 0.0);
    mText->setText(UTF8_CROSS_SYMBOL);
    mText->setPaddings(5, 0, 0, 0);
    mText->resize(mSize);

    mOpacityEffect = new QGraphicsOpacityEffect(this);
    mOpacityEffect->setOpacity(0.0);
    mOpacityEffect->setEnabled(true);
    setGraphicsEffect(mOpacityEffect);

    setFocusPolicy(Qt::FocusPolicy::NoFocus);
    setAutoDefault(false);
    setDefault(false);
}

ToolbarButton::~ToolbarButton() = default;

QSize ToolbarButton::sizeHint() const
{
    return mSize;
}

void ToolbarButton::setColor(QColor color)
{
    mText->setColor(color);
}

void ToolbarButton::paintEvent(QPaintEvent *event)
{
    //QPainter painter(this);
    //painter.setBrush(QColorConstants::Red);
    //painter.setPen(QColorConstants::Red);
    //painter.drawRect(this->rect());
}

void ToolbarButton::mouseMoveEvent(QMouseEvent* event)
{
    QPushButton::mouseMoveEvent(event);
    emit hoverEvent();
}

void ToolbarButton::enterEvent(QEnterEvent* event)
{
    mOpacityEffect->setOpacity(1.0);
}

void ToolbarButton::leaveEvent(QEvent* event)
{
    mOpacityEffect->setOpacity(0.0);
}



