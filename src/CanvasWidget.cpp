/**
* ShibaView
*
* The MIT License (MIT)
* Copyright (c) 2018 Alexey Gruzdev
*/

#include "CanvasWidget.h"

#include <iostream>

#include <QApplication>
#include <qdesktopwidget.h>
#include <QKeyEvent>
#include <QPainter>
#include <QSettings>

enum class BorderPosition
{
    eNone   = 0x0,
    eLeft   = 0x1,
    eRight  = 0x2,
    eTop    = 0x4,
    eBottom = 0x8,
    eTopLeft  = eTop | eLeft,
    eTopRight = eTop | eRight,
    eBotLeft  = eBottom | eLeft,
    eBotRight = eBottom | eRight
};

namespace
{
    Q_CONSTEXPR int kMinSize = 256;
    Q_CONSTEXPR int kFrameThickness = 8;

    const QString kSettingsGeometry   = "canvas/geometry";
    const QString kSettingsFullscreen = "canvas/fullscreen";

    Q_CONSTEXPR
    BorderPosition operator|(const BorderPosition & lh, const BorderPosition & rh)
    {
        return static_cast<BorderPosition>(static_cast<std::underlying_type_t<BorderPosition>>(lh) | static_cast<std::underlying_type_t<BorderPosition>>(rh));
    }

    Q_CONSTEXPR
    BorderPosition operator&(const BorderPosition & lh, const BorderPosition & rh)
    {
        return static_cast<BorderPosition>(static_cast<std::underlying_type_t<BorderPosition>>(lh) & static_cast<std::underlying_type_t<BorderPosition>>(rh));
    }
}


CanvasWidget::CanvasWidget(std::chrono::steady_clock::time_point t)
    : QWidget(nullptr)
    , mHoveredBorder(BorderPosition::eNone)
    , mStartTime(t)
{
    QSettings settings;
    setGeometry(settings.value(kSettingsGeometry, QRect(200, 200, 1280, 720)).toRect());
    mFullScreen = settings.value(kSettingsFullscreen, false).toBool();
    setStyleSheet("background-color:#2B2B2B;");
    setMouseTracking(true);
    mClickGeometry = geometry();
    if (mFullScreen) {
        setGeometry(QApplication::desktop()->screenGeometry());
    }
}

CanvasWidget::~CanvasWidget()
{ }

void CanvasWidget::updateSettings()
{
    QSettings settings;
    settings.setValue(kSettingsGeometry, mClickGeometry);
    settings.setValue(kSettingsFullscreen, mFullScreen);
}

void CanvasWidget::onImageReady(QPixmap p)
{
    if(p.isNull()) {
        close();
        return;
    }
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
    }

    const float dx = mPixmap.width()  * mZoom / 2.0f;
    const float dy = mPixmap.height() * mZoom / 2.0f;

    QTransform viewTransform;
    viewTransform.translate(width() / 2.0f - dx, height() / 2.0f - dy);
    viewTransform.scale(mZoom, mZoom);

    QPainter painter(this);
    painter.setTransform(viewTransform);
    painter.drawPixmap(0, 0, mPixmap);
}

void CanvasWidget::resizeEvent(QResizeEvent * /* event */)
{
    repaint();
}

void CanvasWidget::keyPressEvent(QKeyEvent* event)
{
    if(event->key() == Qt::Key_Escape) {
        close();
    }
}

void CanvasWidget::mousePressEvent(QMouseEvent* event)
{
    QWidget::mousePressEvent(event);
    if(!mFullScreen) {
        if(mHoveredBorder != BorderPosition::eNone) {
            // resise window
            mStretching = true;
            mClickGeometry = geometry();
            mClickX = event->globalX();
            mClickY = event->globalY();
        }
        else if(event->buttons() & Qt::LeftButton) {
            // drag window
            mDragging = true;
            mClickX = event->x();
            mClickY = event->y();
        }
    }
    mClick = true;
}

void CanvasWidget::mouseReleaseEvent(QMouseEvent* event)
{
    QWidget::mouseReleaseEvent(event);
    if(!mFullScreen) {
        mDragging   = false;
        if (mStretching) {
            mStretching = false;
            mFullScreen = false;
        }
        mClickGeometry = geometry();
        updateSettings();
    }
    mClick = false;
}

void CanvasWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    QWidget::mouseDoubleClickEvent(event);
    if(mHoveredBorder == BorderPosition::eNone) {
        if(mFullScreen) {
            setGeometry(mClickGeometry);
            mFullScreen = false;
        } else {
            mClickGeometry = geometry();
            setGeometry(QApplication::desktop()->screenGeometry());
            mFullScreen = true;
        }
        updateSettings();
    }
}

void CanvasWidget::mouseMoveEvent(QMouseEvent* event)
{
    QWidget::mouseMoveEvent(event);
    if(!mFullScreen) {
        if (mDragging) {
            move(event->globalX() - mClickX, event->globalY() - mClickY);
        }
        else if (mStretching) {
            QRect r = mClickGeometry;
            if ((mHoveredBorder & BorderPosition::eLeft) != BorderPosition::eNone) {
                r.setX(std::min(r.x() + event->globalX() - mClickX, r.right() - kMinSize));
            }
            if ((mHoveredBorder & BorderPosition::eRight) != BorderPosition::eNone) {
                r.setWidth(std::max(kMinSize, event->x()));
            }
            if ((mHoveredBorder & BorderPosition::eTop) != BorderPosition::eNone) {
                r.setY(std::min(r.y() + event->globalY() - mClickY, r.bottom() - kMinSize));
            }
            if ((mHoveredBorder & BorderPosition::eBottom) != BorderPosition::eNone) {
                r.setHeight(std::max(kMinSize, event->y()));
            }
            setGeometry(r);
        }
        else {
            const int x = event->x();
            const int y = event->y();
            BorderPosition pos = BorderPosition::eNone;
            if (x <= kFrameThickness) {
                pos = pos | BorderPosition::eLeft;
            }
            if (width() - x <= kFrameThickness) {
                pos = pos | BorderPosition::eRight;
            }
            if (y <= kFrameThickness) {
                pos = pos | BorderPosition::eTop;
            }
            if (height() - y <= kFrameThickness) {
                pos = pos | BorderPosition::eBottom;
            }
            switch (pos) {
            case BorderPosition::eLeft:
            case BorderPosition::eRight:
                setCursor(Qt::SizeHorCursor);
                break;
            case BorderPosition::eTop:
            case BorderPosition::eBottom:
                setCursor(Qt::SizeVerCursor);
                break;
            case BorderPosition::eTopLeft:
            case BorderPosition::eBotRight:
                setCursor(Qt::SizeFDiagCursor);
                break;
            case BorderPosition::eTopRight:
            case BorderPosition::eBotLeft:
                setCursor(Qt::SizeBDiagCursor);
                break;
            default:
                unsetCursor();
                break;
            }
            mHoveredBorder = pos;
        }
    }
}

void CanvasWidget::wheelEvent(QWheelEvent* event)
{
    if(!mClick) {
        const QPoint degrees = event->angleDelta();
        if (!degrees.isNull() && degrees.y() != 0) {
            const float step   = 0.075f;
            const float factor = (degrees.y() > 0) ? 1.0f + step : 1.0f - step;
            mZoom = std::max(0.01f, std::min(mZoom * factor, 100.0f));
            update();
        }
    }
}

