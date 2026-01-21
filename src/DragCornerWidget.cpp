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

#include "DragCornerWidget.h"
#include <QMouseEvent>
#include <QPainter>
#include <iostream>

DragCornerWidget::DragCornerWidget(QWidget *parent, uint32_t size, QColor background)
    : QWidget(parent)
    , mBackgroundColor(background)
{
    resize(size, size);
}

DragCornerWidget::~DragCornerWidget() = default;

void DragCornerWidget::mousePressEvent(QMouseEvent* event)
{
    QWidget::mousePressEvent(event);
    if (event && !isHidden()) {
        mClickLocalPos = event->scenePosition().toPoint();
        mIsDragged = true;
        emit draggingStart();
    }
}

void DragCornerWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (event && !isHidden()) {
        if (!(event->buttons() & Qt::LeftButton)) {
            mIsDragged = false;
            emit draggingStop();
        }
        if (mIsDragged) {
            const auto pos = event->scenePosition().toPoint();
            emit draggingOffset(pos - mClickLocalPos);
        }
    }
}

void DragCornerWidget::mouseReleaseEvent(QMouseEvent* event)
{
    QWidget::mouseReleaseEvent(event);
    if (event && !isHidden()) {
        if (mIsDragged) {
            mIsDragged = false;
            emit draggingStop();
        }
    }
}

void DragCornerWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.setBrush(mBackgroundColor);
    painter.setPen(mBackgroundColor);
    painter.drawRoundedRect(this->rect(), mCornerRadius, mCornerRadius);
}

void DragCornerWidget::keyPressEvent(QKeyEvent* event)
{
    event->ignore();
}

void DragCornerWidget::keyReleaseEvent(QKeyEvent* event)
{
    event->ignore();
}
