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
#include <QWidgetAction>

#include "Global.h"
#include "MenuWidget.h"
#include "TextWidget.h"
#include "ZoomController.h"

#define UTF8_DEGREE "\xc2\xb0"

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
    QString toPercent(float z)
    {
        return QString::number(100.0f * z, 'f', 0) + QString("%");
    }
}


CanvasWidget::CanvasWidget(std::chrono::steady_clock::time_point t)
    : QWidget(nullptr)
    , mHoveredBorder(BorderPosition::eNone)
    , mStartTime(t)
{
    mInfoText = new TextWidget(this);
    mInfoText->move(15, 30);

    mErrorText = new TextWidget(this);

    QGraphicsDropShadowEffect *eff = new QGraphicsDropShadowEffect(this);
    eff->setOffset(-1, 0);
    eff->setBlurRadius(5.0);
    eff->setColor(Qt::black);
    mInfoText->setGraphicsEffect(eff);

    QSettings settings;
    mClickGeometry = settings.value(kSettingsGeometry, QRect(200, 200, 1280, 720)).toRect();
    mFullScreen    = settings.value(kSettingsFullscreen, false).toBool();
    mShowInfo      = settings.value(kSettingsShowInfo, false).toBool();
    mFilteringMode = static_cast<FilteringMode>(settings.value(kSettingsFilterMode, static_cast<int>(FilteringMode::eNone)).toInt());

    mZoomMode = ZoomMode::eFitWidth;

    QPalette palette;
    palette.setColor(QPalette::ColorRole::Window, QColor(0x2B, 0x2B, 0x2B));
    setPalette(palette);

    setMouseTracking(true);

    if (mFullScreen) {
        setFullscreenGeometry();
    }
    else {
        setGeometry(mClickGeometry);
    }

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, &CanvasWidget::onShowContextMenu);

    // Context menu
    {
        const auto makeAction = [this](const QString & text) -> QWidgetAction* {
            auto action = new QWidgetAction(this);
            auto widget = new MenuWidget(text);
            connect(action, &QAction::toggled, widget, &MenuWidget::onActionToggled);
            action->setDefaultWidget(widget);
            return action;
        };

        // Filtering
        {
            auto filteringGroup = new QActionGroup(this);

            auto actNoFilter = makeAction("No filter");
            actNoFilter->setCheckable(true);
            actNoFilter->setActionGroup(filteringGroup);

            auto actAntialiasing = makeAction("Antialiasing");
            actAntialiasing->setCheckable(true);
            actAntialiasing->setActionGroup(filteringGroup);

            switch(mFilteringMode) {
            case FilteringMode::eNone:
                actNoFilter->setChecked(true);
                break;
            case FilteringMode::eAntialiasing:
                actAntialiasing->setChecked(true);
                break;
            }

            connect(actNoFilter,     &QAction::triggered, this, &CanvasWidget::onActNoFilter);
            connect(actAntialiasing, &QAction::triggered, this, &CanvasWidget::onActAntialiasing);

            mContextMenu.addAction(actNoFilter);
            mContextMenu.addAction(actAntialiasing);
            mContextMenu.addSeparator();
        }

        // Rotation
        {
            auto rotationGroup = new QActionGroup(this);

            auto actRotation0 = makeAction(QString::fromUtf8("Rotation 0" UTF8_DEGREE));
            actRotation0->setCheckable(true);
            actRotation0->setActionGroup(rotationGroup);
            actRotation0->setChecked(true);

            auto actRotation90 = makeAction(QString::fromUtf8("Rotation 90" UTF8_DEGREE));
            actRotation90->setCheckable(true);
            actRotation90->setActionGroup(rotationGroup);

            auto actRotation180 = makeAction(QString::fromUtf8("Rotation 180" UTF8_DEGREE));
            actRotation180->setCheckable(true);
            actRotation180->setActionGroup(rotationGroup);

            auto actRotation270 = makeAction(QString::fromUtf8("Rotation -90" UTF8_DEGREE));
            actRotation270->setCheckable(true);
            actRotation270->setActionGroup(rotationGroup);

            connect(actRotation0,   &QAction::triggered, std::bind(&CanvasWidget::onActRotation, this, std::placeholders::_1, Rotation::eDegree0));
            connect(actRotation90,  &QAction::triggered, std::bind(&CanvasWidget::onActRotation, this, std::placeholders::_1, Rotation::eDegree90));
            connect(actRotation180, &QAction::triggered, std::bind(&CanvasWidget::onActRotation, this, std::placeholders::_1, Rotation::eDegree180));
            connect(actRotation270, &QAction::triggered, std::bind(&CanvasWidget::onActRotation, this, std::placeholders::_1, Rotation::eDegree270));

            mContextMenu.addAction(actRotation0);
            mContextMenu.addAction(actRotation90);
            mContextMenu.addAction(actRotation180);
            mContextMenu.addAction(actRotation270);
            mContextMenu.addSeparator();
        }

        const auto actQuit = makeAction("Quit");
        connect(actQuit, &QAction::triggered, this, &QWidget::close);
        mContextMenu.addAction(actQuit);

        mContextMenu.setFont(QFont(Global::defaultFont, 10));
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

void CanvasWidget::onShowContextMenu(const QPoint & p)
{
    mContextMenu.exec(mapToGlobal(p));
}

void CanvasWidget::onImageReady(QSharedPointer<Image> image)
{
    mPendingImage = std::move(image);
    if(!mVisible) {
        show();
        mVisible = false;
    }
    mTransitionRequested = false;
    mErrorText->hide();
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

    const int fitHeight = static_cast<int>(static_cast<int64_t>(fitWidth) * h / w);

    QRect r;
    r.setLeft(zeroX - fitWidth  / 2);
    r.setTop (zeroY - fitHeight / 2);
    r.setWidth(fitWidth);
    r.setHeight(fitHeight);

    return r;
}

void CanvasWidget::invalidateImageExtents(bool keepTransform)
{
    (void)keepTransform;

    const int w = mImage->width();
    const int h = mImage->height();

    mImageRegion    = fitWidth(w, h);
    mZoomController = std::make_unique<ZoomController>(w, mImageRegion.width(), w / kMinZoomRatio, w * kMaxZoomRatio);

    auto infoLines = mImage->info().toLines();
    infoLines.push_back(kZoomLine + toPercent(static_cast<float>(mImageRegion.width()) / mImage->width()));
    mInfoText->setText(infoLines);
}

void CanvasWidget::paintEvent(QPaintEvent * event)
{
    if(mStartup){
#ifdef _MSC_VER
        std::cout << (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - mStartTime).count() / 1e3) << std::endl;
#endif
    }

    QWidget::paintEvent(event);


    if (mPendingImage) {
        mImage = std::move(mPendingImage);
        mPendingImage = nullptr;

        if (mImage->isValid()) {
            invalidateImageExtents(false);
        }
    }

    if (mImage) {
        if (mImage->isValid()) {
            QPainter painter(this);
            if(mFilteringMode == FilteringMode::eAntialiasing) {
                painter.setRenderHint(QPainter::RenderHint::SmoothPixmapTransform, true);
            }
            painter.drawPixmap(mImageRegion, mImage->pixmap());

            if (mShowInfo) {
                mInfoText->show();
            }
            else {
                mInfoText->hide();
            }
        }
        else {
            const auto dstRegion = fitWidth(512, 512);
            QPainter painter(this);
            painter.setBrush(Qt::black);
            painter.drawRect(dstRegion);
            painter.setPen(Qt::red);
            painter.setBrush(Qt::red);
            painter.drawLine(dstRegion.topLeft(), dstRegion.bottomRight());
            painter.drawLine(dstRegion.topRight(), dstRegion.bottomLeft());

            QVector<QString> error;
            error.push_back(mImage->info().path);
            mErrorText->setText(error);
            mErrorText->move(width() / 2 - mErrorText->width() / 2, height() / 2- mErrorText->height() / 2);
            mErrorText->show();
        }
    }

    if(mStartup){
#ifdef _MSC_VER
        std::cout << (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - mStartTime).count() / 1e3) << std::endl;
#endif
        mStartup = false;
    }
}

void CanvasWidget::recalculateZoom()
{
    if(mImage && mImage->isValid()) {
        const int w = mImage->width();
        mZoomController = std::make_unique<ZoomController>(w, mImageRegion.width(), w / kMinZoomRatio, w * kMaxZoomRatio);
        if (mInfoText) {
            mInfoText->setLine(ImageInfo::linesNumber(), kZoomLine + toPercent(static_cast<float>(mImageRegion.width()) / mImage->width()));
        }
    }
}

void CanvasWidget::resizeEvent(QResizeEvent * event)
{
    QWidget::resizeEvent(event);
    updateOffsets();
    if(mZoomMode == ZoomMode::eFitWidth && mImage && mImage->isValid()) {
        mImageRegion = fitWidth(mImage->width(), mImage->height());
        if (mInfoText) {
            mInfoText->setLine(ImageInfo::linesNumber(), kZoomLine + toPercent(static_cast<float>(mImageRegion.width()) / mImage->width()));
        }
    }
    repaint();
}

void CanvasWidget::keyPressEvent(QKeyEvent* event)
{
    QWidget::keyPressEvent(event);
    if (QApplication::keyboardModifiers() == Qt::AltModifier) {
        // Alt + Key
        if(mImage && mImage->isValid()) {
            if (event->key() == Qt::Key_Up) {
                mImage->setRotation(Rotation::eDegree0);
                invalidateImageExtents();
                update();
            }
            else if (event->key() == Qt::Key_Left) {
                mImage->setRotation(Rotation::eDegree270);
                invalidateImageExtents();
                update();
            }
            else if (event->key() == Qt::Key_Right) {
                mImage->setRotation(Rotation::eDegree90);
                invalidateImageExtents();
                update();
            }
            else if (event->key() == Qt::Key_Down) {
                mImage->setRotation(Rotation::eDegree180);
                invalidateImageExtents();
                update();
            }
        }
    }
     else {
         // No Modifiers
        if (event->key() == Qt::Key_Escape) {
            close();
        }
        else if (event->key() == Qt::Key_Tab) {
            mShowInfo = !mShowInfo;
        }
        else if ((event->key() == Qt::Key_Asterisk) && mImage && mImage->isValid()) {
            switch (mZoomMode) {
            case ZoomMode::eCustom:
            case ZoomMode::e100Percent:
                mZoomMode = ZoomMode::eFitWidth;
                mImageRegion = fitWidth(mImage->width(), mImage->height());
                recalculateZoom();
                if(mZoomController) {
                    mZoomController->moveToPosFit();
                }
                break;
            case ZoomMode::eFitWidth:
                mZoomMode = ZoomMode::e100Percent;
                mImageRegion.setLeft(width()  / 2 - mImage->width()  / 2);
                mImageRegion.setTop (height() / 2 - mImage->height() / 2);
                mImageRegion.setWidth(mImage->width());
                mImageRegion.setHeight(mImage->height());
                recalculateZoom();
                if(mZoomController) {
                    mZoomController->moveToPos100();
                }
                break;
            }
            update();
        }
        else if (event->key() == Qt::Key_Plus) {
            zoomToTarget(QPoint(width() / 2, height() / 2), 1);
        }
        else if (event->key() == Qt::Key_Minus) {
            zoomToTarget(QPoint(width() / 2, height() / 2), -1);
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
        else if (event->key() == Qt::Key_Home) {
            if (!mTransitionRequested) {
                mTransitionRequested = true;
                emit eventFirstImage();
            }
        }
        else if (event->key() == Qt::Key_End) {
            if (!mTransitionRequested) {
                mTransitionRequested = true;
                emit eventLastImage();
            }
        }
    }
}

void CanvasWidget::onTransitionCanceled()
{
    mTransitionRequested = false;
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

void CanvasWidget::setFullscreenGeometry()
{
    const auto screen = QApplication::screenAt(mClickGeometry.center());
    if (screen) {
        setGeometry(screen->geometry());
    }
    else {
        setGeometry(QApplication::desktop()->screenGeometry());
    }
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
            setFullscreenGeometry();
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

void CanvasWidget::zoomToTarget(QPoint target, int dir)
{
    if(mZoomController && mImage && mImage->isValid()) {
        mZoomMode = ZoomMode::eCustom;

        const int w = mImageRegion.width();

        const int dw = (dir > 0) ? mZoomController->zoomPlus() : mZoomController->zoomMinus();
        const int dh = dw * mImage->height() / mImage->width();

        mImageRegion.setLeft(static_cast<int>(static_cast<int64_t>(mImageRegion.left() - target.x()) * dw / w + target.x()));
        mImageRegion.setTop (static_cast<int>(static_cast<int64_t>(mImageRegion.top()  - target.y()) * dw / w + target.y()));

        mImageRegion.setWidth(dw);
        mImageRegion.setHeight(dh);

        if (mInfoText) {
            mInfoText->setLine(ImageInfo::linesNumber(), kZoomLine + toPercent(static_cast<float>(dw) / mImage->width()));
        }

        updateOffsets();
        update();
    }
}

void CanvasWidget::wheelEvent(QWheelEvent* event)
{
    if(!mClick) {
        const QPoint degrees = event->angleDelta();
        if (!degrees.isNull() && degrees.y() != 0) {
            mCursorPosition = QPoint(event->x(), event->y());
            zoomToTarget(mCursorPosition, (degrees.y() > 0) ? 1 : -1);
        }
    }
}

// Actions

void CanvasWidget::onActNoFilter(bool checked)
{
    if (checked) {
        mFilteringMode = FilteringMode::eNone;
        update();
    }
}

void CanvasWidget::onActAntialiasing(bool checked)
{
    if (checked) {
        mFilteringMode = FilteringMode::eAntialiasing;
        update();
    }
}

void CanvasWidget::onActRotation(bool checked, Rotation r)
{
    if (checked && mImage && mImage->isValid()) {
        mImage->setRotation(r);
        invalidateImageExtents();
        update();
    }
}
