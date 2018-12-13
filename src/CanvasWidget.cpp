/**
* ShibaView
*
* The MIT License (MIT)
* Copyright (c) 2018 Alexey Gruzdev
*/

#include "CanvasWidget.h"

#include <iostream>

#include <QApplication>
#include <QDesktopWidget>
#include <QKeyEvent>
#include <Qlabel>
#include <QPainter>
#include <QSettings>
#include <QScreen>
#include <QGraphicsDropShadowEffect>

#include <ZoomController.h>

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
    const QString kSettingsShowInfo   = "canvas/info";

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
    mInfoLabel = new QLabel(this);
    mInfoLabel->setMargin(25);
    mInfoLabel->setStyleSheet(R"CSS(
        QLabel { 
            font-size: 14px;
            font-weight: bold;
            background-color : transparent;
            color : white; 
        }
    )CSS");

    QGraphicsDropShadowEffect *eff = new QGraphicsDropShadowEffect(this);
    eff->setOffset(-1, -1);
    eff->setBlurRadius(5.0);
    eff->setColor(Qt::black);
    mInfoLabel->setGraphicsEffect(eff);

    connect(this, &CanvasWidget::eventInfoText, mInfoLabel, &QLabel::setText, Qt::QueuedConnection);
    connect(this, &CanvasWidget::eventInfoText, mInfoLabel, [this](QString){mInfoLabel->adjustSize();}, Qt::QueuedConnection);

    QSettings settings;
    setGeometry(settings.value(kSettingsGeometry, QRect(200, 200, 1280, 720)).toRect());
    mFullScreen = settings.value(kSettingsFullscreen, false).toBool();
    mShowInfo   = settings.value(kSettingsShowInfo, false).toBool();
    setStyleSheet("background-color:#2B2B2B;");
    setMouseTracking(true);
    mClickGeometry = geometry();
    if (mFullScreen) {
        setGeometry(QApplication::desktop()->screenGeometry());
    }
}

CanvasWidget::~CanvasWidget()
{
    try {
        QSettings settings;
        settings.setValue(kSettingsGeometry,   mClickGeometry);
        settings.setValue(kSettingsFullscreen, mFullScreen);
        settings.setValue(kSettingsShowInfo,   mShowInfo);
    }
    catch(...) {
        
    }
}

void CanvasWidget::onImageReady(QPixmap p, const ImageInfo & i)
{
    if(p.isNull()) {
        close();
        return;
    }
    mPendingImage.reset();
    mPendingImage.reset(new QPixmap(p));
    mImageInfo = i;
    if(!mVisible) {
        show();
        mVisible = false;
    }
    update();
}

void CanvasWidget::updateOffsets()
{
    const int w = mImageRegion.width();
    const int h = mImageRegion.height();
    const int zeroX = width()  / 2;
    const int zeroY = height() / 2;
    if (w > width()) {
        if (mImageRegion.left() + w < width()) {
            mImageRegion.moveLeft(width() - w);
        }
        if (mImageRegion.left() > 0) {
            mImageRegion.moveLeft(0);
        }
    }
    else {
        mImageRegion.moveLeft(zeroX - w / 2);
    }
    if(h > height()) {
        if (mImageRegion.top() + h < height()) {
            mImageRegion.moveTop(height() - h);
        }
        if (mImageRegion.top() > 0) {
            mImageRegion.moveTop(0);
        }
    }
    else {
        mImageRegion.moveTop(zeroY - h / 2);
    }
}

void CanvasWidget::paintEvent(QPaintEvent * event)
{
    if(mStartup){
        std::cout << (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - mStartTime).count() / 1e3) << std::endl;
    }

    QWidget::paintEvent(event);


    if(mPendingImage != nullptr) {
        mPixmap = std::move(*mPendingImage);
        mPendingImage = nullptr;

        const int w = mPixmap.width();
        const int h = mPixmap.height();

        const int zeroX = width()  / 2;
        const int zeroY = height() / 2;

        mImageRegion.setLeft(zeroX - w / 2);
        mImageRegion.setTop (zeroY - h / 2);
        mImageRegion.setWidth(w);
        mImageRegion.setHeight(h);

        mZoomController = std::make_unique<ZoomController>(w, 8, 50 * w);

        emit eventInfoText(mImageInfo.toString());
    }

    if(!mPixmap.isNull()) {
        QPainter painter(this);
        painter.drawPixmap(mImageRegion, mPixmap);

        if (mShowInfo) {
            mInfoLabel->show();
        }
        else {
            mInfoLabel->hide();
        }
    }

    if(mStartup){
        std::cout << (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - mStartTime).count() / 1e3) << std::endl;
        mStartup = false;
    }
}

void CanvasWidget::resizeEvent(QResizeEvent * event)
{
    QWidget::resizeEvent(event);
    updateOffsets();
    repaint();
}

void CanvasWidget::keyPressEvent(QKeyEvent* event)
{
    QWidget::keyPressEvent(event);
    if(event->key() == Qt::Key_Escape) {
        close();
    }
    else if(event->key() == Qt::Key_Tab) {
        mShowInfo = !mShowInfo;
    }
}

void CanvasWidget::mousePressEvent(QMouseEvent* event)
{
    QWidget::mousePressEvent(event);
    if(!mFullScreen && mHoveredBorder != BorderPosition::eNone) {
        // resise window
        mStretching = true;
        mClickGeometry = geometry();
        mClickX = event->globalX();
        mClickY = event->globalY();
    }
    else if(!mFullScreen && event->buttons() & Qt::LeftButton) {
        // drag window
        mDragging = true;
        mClickX = event->x();
        mClickY = event->y();
    }
    else if(event->button() & Qt::MiddleButton) {
        // drag image
        mBrowsing = true;
        mClickX = event->x();
        mClickY = event->y();
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
    }
    mBrowsing = false;
    mClick = false;
}

void CanvasWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    QWidget::mouseDoubleClickEvent(event);
    if((event->button() & Qt::LeftButton) && (mHoveredBorder == BorderPosition::eNone)) {
        if(mFullScreen) {
            setGeometry(mClickGeometry);
            mFullScreen = false;
        } else {
            mClickGeometry = geometry();
            const auto screen = QApplication::screenAt(mClickGeometry.center());
            if (screen) {
                setGeometry(screen->geometry());
            }
            else {
                setGeometry(QApplication::desktop()->screenGeometry());
            }
            mFullScreen = true;
        }
        updateOffsets();
    }
}

void CanvasWidget::mouseMoveEvent(QMouseEvent* event)
{
    QWidget::mouseMoveEvent(event);
    
    if (!mFullScreen && mDragging) {
        move(event->globalX() - mClickX, event->globalY() - mClickY);
    }
    else if (mBrowsing) {
        mImageRegion.translate(event->x() - mClickX, event->y() - mClickY);
        mClickX   = event->x();
        mClickY   = event->y();
        updateOffsets();
        repaint();
    }
    else if (!mFullScreen && mStretching) {
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
        updateOffsets();
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
    
    mCursorPosition = QPoint(event->x(), event->y());
}

void CanvasWidget::wheelEvent(QWheelEvent* event)
{
    if(!mClick) {
        const QPoint degrees = event->angleDelta();
        if (!degrees.isNull() && degrees.y() != 0) {
            if(mZoomController) {
                const int w = mImageRegion.width();

                const int dw = (degrees.y() > 0) ? mZoomController->zoomPlus() : mZoomController->zoomMinus();
                const int dh = dw * mPixmap.height() / mPixmap.width();

                mImageRegion.setLeft((mImageRegion.left() - mCursorPosition.x()) * dw / w + mCursorPosition.x());
                mImageRegion.setTop ((mImageRegion.top()  - mCursorPosition.y()) * dw / w + mCursorPosition.y());

                mImageRegion.setWidth(dw);
                mImageRegion.setHeight(dh);

                updateOffsets();
                update();
            }
        }
    }
}

