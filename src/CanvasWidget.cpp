/**
 * @file
 *
 * Copyright 2018-2019 Alexey Gruzdev
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
#include "UniqueTick.h"

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

    Q_CONSTEXPR int kTextPaddingLeft = 15;
    Q_CONSTEXPR int kTextPaddingTop  = 30;

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
    mInfoText->move(kTextPaddingLeft, kTextPaddingTop);
    mInfoText->enableShadow();

    mErrorText = new TextWidget(this);

    QSettings settings;
    mClickGeometry = settings.value(kSettingsGeometry, QRect(200, 200, 1280, 720)).toRect();
    mFullScreen    = settings.value(kSettingsFullscreen, false).toBool();
    mShowInfo      = settings.value(kSettingsShowInfo, false).toBool();
    mFilteringMode = static_cast<FilteringMode>(settings.value(kSettingsFilterMode, static_cast<int>(FilteringMode::eNone)).toInt());
    mZoomMode      = static_cast<ZoomMode>(settings.value(kSettingsZoomMode, static_cast<int>(ZoomMode::eFree)).toInt());

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
}

CanvasWidget::~CanvasWidget()
{
    try {
        QSettings settings;
        settings.setValue(kSettingsGeometry,   mClickGeometry);
        settings.setValue(kSettingsFullscreen, mFullScreen);
        settings.setValue(kSettingsShowInfo,   mShowInfo);
        settings.setValue(kSettingsFilterMode, static_cast<int>(mFilteringMode));
        settings.setValue(kSettingsZoomMode,   static_cast<int>(mZoomMode));
    }
    catch(...) {
        
    }
}

QWidgetAction* CanvasWidget::createMenuAction(const QString & text)
{
    auto action = new QWidgetAction(this);
    auto widget = new MenuWidget(text);
    connect(action, &QAction::toggled, widget, &MenuWidget::onActionToggled);
    action->setDefaultWidget(widget);
    return action;
}

QMenu* CanvasWidget::createContextMenu()
{
    QMenu* menu = new QMenu(this);

    // Filtering
    {
        auto filteringGroup = new QActionGroup(this);

        auto actNoFilter = createMenuAction("No filter");
        actNoFilter->setCheckable(true);
        actNoFilter->setActionGroup(filteringGroup);

        auto actAntialiasing = createMenuAction("Antialiasing");
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

        menu->addAction(actNoFilter);
        menu->addAction(actAntialiasing);
        menu->addSeparator();
    }

    // Rotation
    {
        initRotationActions();

        menu->addAction(mActRotate0);
        menu->addAction(mActRotate90);
        menu->addAction(mActRotate180);
        menu->addAction(mActRotate270);
        menu->addSeparator();
    }

    // Zoom
    {
        initZoomActions();

        menu->addAction(mActZoom[toIndex(ZoomMode::eFree)]);
        menu->addAction(mActZoom[toIndex(ZoomMode::eFitWindow)]);
        menu->addAction(mActZoom[toIndex(ZoomMode::eFixed)]);
        menu->addSeparator();
    }

    const auto actQuit = createMenuAction("Quit");
    connect(actQuit, &QAction::triggered, this, &QWidget::close);
    menu->addAction(actQuit);

    return menu;
}

void CanvasWidget::initRotationActions()
{
    if (!mActGroupRotation) {
        mActGroupRotation = new QActionGroup(this);

        mActRotate0 = createMenuAction(QString::fromUtf8("Rotation 0" UTF8_DEGREE));
        mActRotate0->setCheckable(true);
        mActRotate0->setActionGroup(mActGroupRotation);
        mActRotate0->setChecked(true);

        mActRotate90 = createMenuAction(QString::fromUtf8("Rotation 90" UTF8_DEGREE));
        mActRotate90->setCheckable(true);
        mActRotate90->setActionGroup(mActGroupRotation);

        mActRotate180 = createMenuAction(QString::fromUtf8("Rotation 180" UTF8_DEGREE));
        mActRotate180->setCheckable(true);
        mActRotate180->setActionGroup(mActGroupRotation);

        mActRotate270 = createMenuAction(QString::fromUtf8("Rotation -90" UTF8_DEGREE));
        mActRotate270->setCheckable(true);
        mActRotate270->setActionGroup(mActGroupRotation);

        connect(mActRotate0,   &QAction::triggered, std::bind(&CanvasWidget::onActRotation, this, std::placeholders::_1, Rotation::eDegree0));
        connect(mActRotate90,  &QAction::triggered, std::bind(&CanvasWidget::onActRotation, this, std::placeholders::_1, Rotation::eDegree90));
        connect(mActRotate180, &QAction::triggered, std::bind(&CanvasWidget::onActRotation, this, std::placeholders::_1, Rotation::eDegree180));
        connect(mActRotate270, &QAction::triggered, std::bind(&CanvasWidget::onActRotation, this, std::placeholders::_1, Rotation::eDegree270));
    }
}

void CanvasWidget::initZoomActions()
{
    if (!mActGroupZoom) {
        mActGroupZoom = new QActionGroup(this);

        mActZoom[toIndex(ZoomMode::eFree)] = createMenuAction(QString::fromUtf8("Free zoom"));
        mActZoom[toIndex(ZoomMode::eFree)]->setCheckable(true);
        mActZoom[toIndex(ZoomMode::eFree)]->setActionGroup(mActGroupZoom);

        mActZoom[toIndex(ZoomMode::eFitWindow)] = createMenuAction(QString::fromUtf8("Fit window"));
        mActZoom[toIndex(ZoomMode::eFitWindow)]->setCheckable(true);
        mActZoom[toIndex(ZoomMode::eFitWindow)]->setActionGroup(mActGroupZoom);

        mActZoom[toIndex(ZoomMode::eFixed)] = createMenuAction(QString::fromUtf8("Fix zoom"));
        mActZoom[toIndex(ZoomMode::eFixed)]->setCheckable(true);
        mActZoom[toIndex(ZoomMode::eFixed)]->setActionGroup(mActGroupZoom);

        mActZoom[toIndex(mZoomMode)]->setChecked(true);

        connect(mActZoom[toIndex(ZoomMode::eFree)],      &QAction::triggered, std::bind(&CanvasWidget::onActZoomMode, this, std::placeholders::_1, ZoomMode::eFree));
        connect(mActZoom[toIndex(ZoomMode::eFitWindow)], &QAction::triggered, std::bind(&CanvasWidget::onActZoomMode, this, std::placeholders::_1, ZoomMode::eFitWindow));
        connect(mActZoom[toIndex(ZoomMode::eFixed)],     &QAction::triggered, std::bind(&CanvasWidget::onActZoomMode, this, std::placeholders::_1, ZoomMode::eFixed));
    }
}

void CanvasWidget::onShowContextMenu(const QPoint & p)
{
    try {
        // Deferred initialization
        if (!mContextMenu) {
            mContextMenu = createContextMenu();
        }
        if (mContextMenu) {
            mContextMenu->exec(mapToGlobal(p));
        }
    }
    catch(std::exception & err) {
        qWarning() << QString("CanvasWidget[onShowContextMenu]: ") + QString(err.what());
    }
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
    if (mImage && !mImage->isNull()) {
        QRect imageRegion = calculateImageRegion();

        const int w = imageRegion.width();
        const int h = imageRegion.height();

        if (w > width()) {
            if (imageRegion.left() + w < width()) {
                mOffset.rx() += width() - imageRegion.left() - w;
            }
            if (imageRegion.left() > 0) {
                mOffset.rx() -= imageRegion.left();
            }
        }
        else {
            mOffset.setX(0);
        }
        if(h > height()) {
            if (imageRegion.top() + h < height()) {
                mOffset.ry() += height() - imageRegion.top() - h;
            }
            if (imageRegion.top() > 0) {
                mOffset.ry() -= imageRegion.top();
            }
        }
        else {
            mOffset.setY(0);
        }
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

QRect CanvasWidget::calculateImageRegion() const
{
    const int w = mImage->width();
    const int h = mImage->height();

    int dw{ 0 }, dh{ 0 };
    switch(mImage->rotation()) {
    case Rotation::eDegree0:
    case Rotation::eDegree180:
        dw = mZoomController->getValue();
        dh = dw * h / w;
        break;
    case Rotation::eDegree90:
    case Rotation::eDegree270:
        dh = mZoomController->getValue();
        dw = dh * w / h;
        break;
    }

    const int zeroX = width()  / 2 + mOffset.x();
    const int zeroY = height() / 2 + mOffset.y();

    QRect r;
    r.setLeft(zeroX - dw / 2);
    r.setTop (zeroY - dh / 2);
    r.setWidth(dw);
    r.setHeight(dh);

    return r;
}

void CanvasWidget::updateZoomLabel()
{
    if (mZoomController && mInfoText) {
        mInfoText->setLine(ImageInfo::linesNumber(), kZoomLine + toPercent(mZoomController->getFactor()));
    }
}

void CanvasWidget::repositionPageText()
{
    assert(mPageText != nullptr);
    mPageText->move(kTextPaddingLeft, height() - mPageText->height() * 2);
}

void CanvasWidget::resetOffsets()
{
    mOffset = { 0, 0 };
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
        mCurrPage = Image::kNonePage;

        if (!mImage->isNull()) {
            const auto fitRect = fitWidth(mImage->sourceWidth(), mImage->sourceHeight());

            if(!mZoomController) {
                mZoomController = std::make_unique<ZoomController>(mImage->sourceWidth(), fitRect.width());
                if (mZoomMode == ZoomMode::eFree && mImage->width() <= static_cast<size_t>(width()) && mImage->height() <= static_cast<size_t>(height())) {
                    mZoomController->moveToIdentity();
                }
                else {
                    mZoomController->moveToFit();
                }
                resetOffsets();
            }
            else {
                if (mZoomMode == ZoomMode::eFixed) {
                    mZoomController->rebase(mImage->sourceWidth(), fitRect.width());
                }
                else {
                    mZoomController = std::make_unique<ZoomController>(mImage->sourceWidth(), fitRect.width());
                    if (mZoomMode == ZoomMode::eFree && mImage->width() <= static_cast<size_t>(width()) && mImage->height() <= static_cast<size_t>(height())) {
                        mZoomController->moveToIdentity();
                    }
                    else {
                        mZoomController->moveToFit();
                    }
                    resetOffsets();
                }
            }
            
            if (mImage->pagesCount() > 1) {
                if (mPageText == nullptr) {
                    mPageText = new TextWidget(this);
                    mPageText->enableShadow();
                }
                mPageText->setText("Page 1/" + QString::number(mImage->pagesCount()));
                repositionPageText();
            }
            else {
                if (mPageText) {
                    delete mPageText;
                    mPageText = nullptr;
                }
            }

            if (mZoomController && mInfoText) {
                auto infoLines = mImage->info().toLines();
                infoLines.push_back(kZoomLine + toPercent(mZoomController->getFactor()));
                mInfoText->setText(infoLines);
            }
        }
    }

    if (mImage) {
        if (!mImage->isNull()) {
            QPainter painter(this);
            if (mFilteringMode == FilteringMode::eAntialiasing) {
                painter.setRenderHint(QPainter::RenderHint::SmoothPixmapTransform, true);
            }

            Image::PageInfo page;
            painter.drawPixmap(calculateImageRegion(), mImage->pixmap(&page));

            if (mShowInfo) {
                mInfoText->show();
                if (mPageText) {
                    mPageText->setText("Page " + QString::number(page.index + 1) + "/" + QString::number(mImage->pagesCount()));
                    mPageText->show();
                }
            }
            else {
                mInfoText->hide();
                if (mPageText) {
                    mPageText->hide();
                }
            }

            if ((page.index != mCurrPage) && (mImage->pagesCount() > 1)) {
                new UniqueTick(mImage->id(), page.duration, this, &CanvasWidget::onAnimationTick, this);
            }
            mCurrPage = page.index;
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
    if(mZoomController && mImage && !mImage->isNull()) {
        const auto fitRect = fitWidth(mImage->sourceWidth(), mImage->sourceHeight());
        mZoomController->setFitValue(fitRect.width());
        updateZoomLabel();
    }
}

void CanvasWidget::resizeEvent(QResizeEvent * event)
{
    QWidget::resizeEvent(event);
    if (mImage && !mImage->isNull()) {
        updateOffsets();
        if (mZoomController && (mZoomMode == ZoomMode::eFitWindow)) {
            //mImageRegion = fitWidth(mImage->width(), mImage->height());
            const auto r = fitWidth(mImage->width(), mImage->height());
            mZoomController->setFitValue(r.width());
            mZoomController->moveToFit();
            updateZoomLabel();
        }
    }
    if (mPageText) {
        repositionPageText();
    }
    repaint();
}

void CanvasWidget::keyPressEvent(QKeyEvent* event)
{
    QWidget::keyPressEvent(event);
    if (QApplication::keyboardModifiers() == Qt::AltModifier) {
        // Alt + Key
        if(mImage && !mImage->isNull()) {
            if (event->key() == Qt::Key_Up) {
                initRotationActions();
                if (mActRotate0) {
                    mActRotate0->trigger();
                }
            }
            else if (event->key() == Qt::Key_Left) {
                initRotationActions();
                if (mActRotate270) {
                    mActRotate270->trigger();
                }
            }
            else if (event->key() == Qt::Key_Right) {
                initRotationActions();
                if (mActRotate90) {
                    mActRotate90->trigger();
                }
            }
            else if (event->key() == Qt::Key_Down) {
                initRotationActions();
                if (mActRotate180) {
                    mActRotate180->trigger();
                }
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
        else if ((event->key() == Qt::Key_Asterisk) && mImage && !mImage->isNull()) {
            if (mZoomController) {
                initZoomActions();
                if(mZoomMode != ZoomMode::eFitWindow) {
                    mActZoom[toIndex(ZoomMode::eFitWindow)]->trigger();
                }
                else {
                    mZoomController->moveToIdentity();
                    mActZoom[toIndex(ZoomMode::eFree)]->trigger();
                }
                updateZoomLabel();
                resetOffsets();
                update();
            }
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
        mClickPos = event->globalPos();
    }
    else if(!mFullScreen && event->buttons() & Qt::LeftButton) {
        // drag window
        mDragging = true;
        mClickPos = event->pos();
    }
    else if(event->button() & Qt::MiddleButton) {
        // drag image
        mBrowsing = true;
        mClickPos = event->pos();
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
        if (mImage && !mImage->isNull()) {
            updateOffsets();
            // After full screen
            recalculateZoom();
        }
    }
}

void CanvasWidget::mouseMoveEvent(QMouseEvent* event)
{
    QWidget::mouseMoveEvent(event);
    
    if (!mFullScreen && mDragging) {
        move(event->globalX() - mClickPos.x(), event->globalY() - mClickPos.y());
    }
    else if (mBrowsing) {
        mOffset += event->pos() - mClickPos;
        mClickPos = event->pos();
        updateOffsets();
        repaint();
    }
    else if (!mFullScreen && mStretching) {
        QRect r = mClickGeometry;
        if ((mHoveredBorder & BorderPosition::eLeft) != BorderPosition::eNone) {
            r.setX(std::min(r.x() + event->globalX() - mClickPos.x(), r.right() - kMinSize));
        }
        if ((mHoveredBorder & BorderPosition::eRight) != BorderPosition::eNone) {
            r.setWidth(std::max(kMinSize, event->x()));
        }
        if ((mHoveredBorder & BorderPosition::eTop) != BorderPosition::eNone) {
            r.setY(std::min(r.y() + event->globalY() - mClickPos.y(), r.bottom() - kMinSize));
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
    if(mZoomController && mImage && !mImage->isNull()) {

        const int w = mZoomController->getValue();

        if (dir > 0) {
            mZoomController->zoomPlus();
        }
        else { 
            mZoomController->zoomMinus();
        }

        const int dw = mZoomController->getValue();

        const auto f = static_cast<double>(dw) / w;
        const int32_t dx = static_cast<int32_t>(std::floor((target.x() - mOffset.x() - width()  / 2) * f + width()  / 2 + mOffset.x() - target.x()));
        const int32_t dy = static_cast<int32_t>(std::floor((target.y() - mOffset.y() - height() / 2) * f + height() / 2 + mOffset.y() - target.y()));

        mOffset.rx() -= dx;
        mOffset.ry() -= dy;

        if (mZoomMode == ZoomMode::eFitWindow) {
            mActZoom[toIndex(ZoomMode::eFree)]->trigger();
        }

        updateOffsets();
        updateZoomLabel();
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

void CanvasWidget::onActRotation(bool checked, Rotation rot)
{
    if (checked && mImage && !mImage->isNull()) {
        const auto oldRot = mImage->rotation();
        if(oldRot != rot) {

            mImage->setRotation(rot);
            const auto r = fitWidth(mImage->width(), mImage->height());
            switch(rot) {
                case Rotation::eDegree0:
                case Rotation::eDegree180:
                    mZoomController->setFitValue(r.width());
                    break;
                case Rotation::eDegree90:
                case Rotation::eDegree270:
                    mZoomController->setFitValue(r.height());
                    break;
            }

            const auto dr = static_cast<int>(rot) - static_cast<int>(oldRot);
            if (std::abs(dr) == 180) {
                mOffset = { -mOffset.x(), -mOffset.y() };
            }
            else if(dr == 90 || dr == -270) {
                mOffset = { -mOffset.y(),  mOffset.x() };
            }
            else if(dr == -90 || dr == 270) {
                mOffset = {  mOffset.y(), -mOffset.x() };
            }
            else {
                assert(false && "Unsupported angle");
                resetOffsets();
            }

            if (mZoomMode == ZoomMode::eFitWindow) {
                mZoomController->moveToFit();
            }

            updateOffsets();
            updateZoomLabel();
            update();
        }
    }
}

void CanvasWidget::onActZoomMode(bool checked, ZoomMode z)
{
    if (checked && mZoomController) {
        mZoomMode = z;
        switch(mZoomMode) {
        case ZoomMode::eFitWindow: {
            mZoomController->moveToFit();
            updateZoomLabel();
            update();
        } break;

        default:
        case ZoomMode::eFixed:
        case ZoomMode::eFree:
            break;
        }
    }
    
}

void CanvasWidget::onAnimationTick(uint64_t imgId)
{
    if (mImage && mImage->id() == imgId && !mImage->isNull()) {
        mImage->readNextPage();
        update();
    }
}
