/**
* ShibaView
*
* The MIT License (MIT)
* Copyright (c) 2018 Alexey Gruzdev
*/

#include "CanvasWidget.h"

#include <cmath>
#include <iostream>

#include <QAction>
#include <QApplication>
#include <QDesktopWidget>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QSettings>
#include <QScreen>
#include <QGraphicsDropShadowEffect>
#include <QRawFont>

#include "TextWidget.h"
#include "ZoomController.h"
#include "ViewerApplication.h"

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

    Q_CONSTEXPR int kMinZoomRatio = 30;
    Q_CONSTEXPR int kMaxZoomRatio = 30;

    const QString kSettingsGeometry   = "canvas/geometry";
    const QString kSettingsFullscreen = "canvas/fullscreen";
    const QString kSettingsShowInfo   = "canvas/info";
    const QString kSettingsZoomMode   = "canvas/zoom";
    const QString kSettingsFilterMode = "canvas/filtering";

    const QString kZoomLine = "Zoom: ";

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

    inline
    QString zoomPercents(float z)
    {
        return QString::number(100.0f * z, 'f', 0) + QString("%");
    }
}


CanvasWidget::CanvasWidget(ViewerApplication* app, std::chrono::steady_clock::time_point t)
    : QWidget(nullptr)
    , mParentApplication(app)
    , mHoveredBorder(BorderPosition::eNone)
    , mStartTime(t)
{
    mInfoText = new TextWidget(this);
    mInfoText->move(15, 30);

    QGraphicsDropShadowEffect *eff = new QGraphicsDropShadowEffect(this);
    eff->setOffset(-1, 0);
    eff->setBlurRadius(5.0);
    eff->setColor(Qt::black);
    mInfoText->setGraphicsEffect(eff);

    QSettings settings;
    setGeometry(settings.value(kSettingsGeometry, QRect(200, 200, 1280, 720)).toRect());
    mFullScreen    = settings.value(kSettingsFullscreen, false).toBool();
    mShowInfo      = settings.value(kSettingsShowInfo, false).toBool();
    mFilteringMode = static_cast<FilteringMode>(settings.value(kSettingsFilterMode, static_cast<int>(FilteringMode::eNone)).toInt());

    mZoomMode = ZoomMode::eFitWidth;

    QPalette palette;
    palette.setColor(QPalette::ColorRole::Window, QColor(0x2B, 0x2B, 0x2B));
    setPalette(palette);

    setMouseTracking(true);
    mClickGeometry = geometry();
    if (mFullScreen) {
        setGeometry(QApplication::desktop()->screenGeometry());
    }

    setContextMenuPolicy(Qt::ActionsContextMenu);

    mActNoFilter = std::make_unique<QAction>("No filter", this);
    mActNoFilter->setStatusTip("Disable image filtering");
    mActNoFilter->setCheckable(true);
    addAction(mActNoFilter.get());

    mActAntialiasing = std::make_unique<QAction>("Antialiasing", this);
    mActAntialiasing->setStatusTip("Default image smoothing");
    mActAntialiasing->setCheckable(true);
    addAction(mActAntialiasing.get());

    mActGroupFiltering = std::make_unique<QActionGroup>(this);
    mActGroupFiltering->addAction(mActNoFilter.get());
    mActGroupFiltering->addAction(mActAntialiasing.get());

    switch(mFilteringMode) {
    case FilteringMode::eNone:
        mActNoFilter->setChecked(true);
        break;
    case FilteringMode::eAntialiasing:
        mActAntialiasing->setChecked(true);
        break;
    }

    connect(mActNoFilter.get(), &QAction::triggered, this, &CanvasWidget::onActNoFilter);
    connect(mActAntialiasing.get(), &QAction::triggered, this, &CanvasWidget::onActAntialiasing);

    if(mParentApplication) {
        connect(this, &CanvasWidget::eventNextImage, mParentApplication, &ViewerApplication::onNextImage, Qt::QueuedConnection);
        connect(this, &CanvasWidget::eventPrevImage, mParentApplication, &ViewerApplication::onPrevImage, Qt::QueuedConnection);
    }
}

CanvasWidget::~CanvasWidget()
{
    try {
        QSettings settings;
        settings.setValue(kSettingsGeometry,   mClickGeometry);
        settings.setValue(kSettingsFullscreen, mFullScreen);
        settings.setValue(kSettingsShowInfo,   mShowInfo);
        settings.setValue(kSettingsFilterMode, static_cast<int>(mFilteringMode));
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
    mTransitionRequested = false;
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

QRect CanvasWidget::fitWidth(int w, int h) const
{
    const int zeroX = width()  / 2;
    const int zeroY = height() / 2;

    const float kx = static_cast<float>(width())  / w;
    const float ky = static_cast<float>(height()) / h;
    const int fitWidth = static_cast<int>(std::floor(std::min(kx, ky) * w));

    const int fitHeight = static_cast<int>(static_cast<int64_t>(fitWidth) * mPixmap.height() / mPixmap.width());

    QRect r;
    r.setLeft(zeroX - fitWidth  / 2);
    r.setTop (zeroY - fitHeight / 2);
    r.setWidth(fitWidth);
    r.setHeight(fitHeight);

    return r;
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

        mImageRegion    = fitWidth(w, h);
        mZoomController = std::make_unique<ZoomController>(w, mImageRegion.width(), w / kMinZoomRatio, w * kMaxZoomRatio);

        auto infoLines = mImageInfo.toLines();
        infoLines.push_back(kZoomLine + zoomPercents(static_cast<float>(mImageRegion.width()) / w));
        mInfoText->setText(infoLines);
    }

    if(!mPixmap.isNull()) {
        QPainter painter(this);
        if(mFilteringMode == FilteringMode::eAntialiasing) {
            painter.setRenderHint(QPainter::RenderHint::SmoothPixmapTransform, true);
        }
        painter.drawPixmap(mImageRegion, mPixmap);

        if (mShowInfo) {
            mInfoText->show();
        }
        else {
            mInfoText->hide();
        }
    }

    if(mStartup){
        std::cout << (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - mStartTime).count() / 1e3) << std::endl;
        mStartup = false;
    }
}

void CanvasWidget::recalculateZoom()
{
    if(!mPixmap.isNull()) {
        const int w = mPixmap.width();
        mZoomController = std::make_unique<ZoomController>(w, mImageRegion.width(), w / kMinZoomRatio, w * kMaxZoomRatio);
        if (mInfoText) {
            mInfoText->setLine(ImageInfo::linesNumber(), kZoomLine + zoomPercents(static_cast<float>(mImageRegion.width()) / mPixmap.width()));
        }
    }
}

void CanvasWidget::resizeEvent(QResizeEvent * event)
{
    QWidget::resizeEvent(event);
    updateOffsets();
    if(mZoomMode == ZoomMode::eFitWidth && !mPixmap.isNull()) {
        mImageRegion = fitWidth(mPixmap.width(), mPixmap.height());
        if (mInfoText) {
            mInfoText->setLine(ImageInfo::linesNumber(), kZoomLine + zoomPercents(static_cast<float>(mImageRegion.width()) / mPixmap.width()));
        }
    }
    repaint();
}

void CanvasWidget::keyPressEvent(QKeyEvent* event)
{
    QWidget::keyPressEvent(event);
    if (event->key() == Qt::Key_Escape) {
        close();
    }
    else if (event->key() == Qt::Key_Tab) {
        mShowInfo = !mShowInfo;
    }
    else if ((event->key() == Qt::Key_Asterisk) && !mPixmap.isNull()) {
        switch (mZoomMode) {
        case ZoomMode::eCustom:
        case ZoomMode::e100Percent:
            mZoomMode = ZoomMode::eFitWidth;
            mImageRegion = fitWidth(mPixmap.width(), mPixmap.height());
            recalculateZoom();
            if(mZoomController) {
                mZoomController->moveToPosFit();
            }
            break;
        case ZoomMode::eFitWidth:
            mZoomMode = ZoomMode::e100Percent;
            mImageRegion.setLeft(width()  / 2 - mPixmap.width()  / 2);
            mImageRegion.setTop (height() / 2 - mPixmap.height() / 2);
            mImageRegion.setWidth(mPixmap.width());
            mImageRegion.setHeight(mPixmap.height());
            recalculateZoom();
            if(mZoomController) {
                mZoomController->moveToPos100();
            }
            break;
        }
        update();
    }
    else if (event->key() == Qt::Key_Left) {
        if (!mTransitionRequested) {
            mTransitionRequested = true;
            emit eventPrevImage();
        }
    }
    else if (event->key() == Qt::Key_Right) {
        if (!mTransitionRequested) {
            mTransitionRequested = true;
            emit eventNextImage();
        }
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

            // After stretching
            recalculateZoom();
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
        // After full screen
        recalculateZoom();
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
            if(mZoomController && !mPixmap.isNull()) {
                mZoomMode = ZoomMode::eCustom;

                const int w = mImageRegion.width();

                const int dw = (degrees.y() > 0) ? mZoomController->zoomPlus() : mZoomController->zoomMinus();
                const int dh = dw * mPixmap.height() / mPixmap.width();

                mImageRegion.setLeft(static_cast<int>(static_cast<int64_t>(mImageRegion.left() - mCursorPosition.x()) * dw / w + mCursorPosition.x()));
                mImageRegion.setTop (static_cast<int>(static_cast<int64_t>(mImageRegion.top()  - mCursorPosition.y()) * dw / w + mCursorPosition.y()));

                mImageRegion.setWidth(dw);
                mImageRegion.setHeight(dh);

                if (mInfoText) {
                    mInfoText->setLine(ImageInfo::linesNumber(), kZoomLine + zoomPercents(static_cast<float>(dw) / mPixmap.width()));
                }

                updateOffsets();
                update();
            }
        }
    }
}

// Actions

void CanvasWidget::onActNoFilter(bool checked)
{
    if(checked) {
        mFilteringMode = FilteringMode::eNone;
        update();
    }
}

void CanvasWidget::onActAntialiasing(bool checked)
{
    if(checked) {
        mFilteringMode = FilteringMode::eAntialiasing;
        update();
    }
}
