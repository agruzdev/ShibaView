/**
* ShibaView
*
* The MIT License (MIT)
* Copyright (c) 2018 Alexey Gruzdev
*/

#include "CanvasWidget.h"

#include <iostream>

#include <QPainter>
#include <QKeyEvent>
#include <QDebug>

#include "ImageLoader.h"

CanvasWidget::CanvasWidget(std::chrono::steady_clock::time_point t)
    : QWidget(nullptr)
    , mStartTime(std::move(t))
{ }

CanvasWidget::~CanvasWidget()
{ }

void CanvasWidget::onImageReady(QPixmap p)
{
    mPendingImage.reset();
    mPendingImage.reset(new QPixmap(p));
    if(!mVisible) {
        show();
        mVisible = false;
    }
    update();
}

void CanvasWidget::paintEvent(QPaintEvent * /* event */)
{
    if(mStartup){
        std::cout << (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - mStartTime).count() / 1e3) << std::endl;
        mStartup = false;
    }

    if(mPendingImage != nullptr) {
        mPixmap = std::move(*mPendingImage);
        mPendingImage = nullptr;
        resize(mPixmap.width(), mPixmap.height());
    }

    QPainter painter(this);
    painter.drawPixmap(QPoint(), mPixmap);
}

void CanvasWidget::resizeEvent(QResizeEvent * /* event */)
{

}

void CanvasWidget::keyPressEvent(QKeyEvent* event)
{
    if(event->key() == Qt::Key_Escape) {
        close();
    }
}

void CanvasWidget::mousePressEvent(QMouseEvent* event) {
    mClickX = event->x();
    mClickY = event->y();
}

void CanvasWidget::mouseMoveEvent(QMouseEvent* event) {
    move(event->globalX() - mClickX, event->globalY() - mClickY);
}



