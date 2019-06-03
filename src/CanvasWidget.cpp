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
#include <QToolTip>
#include <QTransform>
#include <QGraphicsDropShadowEffect>
#include <QRawFont>
#include <QWidgetAction>

#include "Global.h"
#include "MenuWidget.h"
#include "TextWidget.h"
#include "ZoomController.h"
#include "UniqueTick.h"
#include "ImageProcessor.h"

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

    const QString kSettingsGeometry    = "canvas/geometry";
    const QString kSettingsFullscreen  = "canvas/fullscreen";
    const QString kSettingsShowInfo    = "canvas/info";
    const QString kSettingsZoomMode    = "canvas/zoom";
    const QString kSettingsFilterMode  = "canvas/filtering";
    const QString kSettingsToneMapping = "canvas/tonemapping";

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
    mFilteringMode = static_cast<FilteringMode>(settings.value(kSettingsFilterMode, static_cast<int32_t>(FilteringMode::eNone)).toInt());
    mZoomMode      = static_cast<ZoomMode>(settings.value(kSettingsZoomMode, static_cast<int32_t>(ZoomMode::eFree)).toInt());

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

    mActRotate      = std::async(std::launch::deferred, [this]{ return initRotationActions();    });
    mActZoom        = std::async(std::launch::deferred, [this]{ return initZoomActions();        });
    mActToneMapping = std::async(std::launch::deferred, [this]{ return initToneMappingActions(); });

    connect(this, &QWidget::customContextMenuRequested, this, &CanvasWidget::onShowContextMenu);

    mImageProcessor = std::make_unique<ImageProcessor>();
    mImageProcessor->setToneMappingMode(static_cast<FIE_ToneMapping>(settings.value(kSettingsToneMapping, static_cast<int32_t>(FIE_ToneMapping::FIETMO_NONE)).toInt()));
}

CanvasWidget::~CanvasWidget()
{
    assert(mImageProcessor); // never null
    try {
        QSettings settings;
        settings.setValue(kSettingsGeometry,   mClickGeometry);
        settings.setValue(kSettingsFullscreen, mFullScreen);
        settings.setValue(kSettingsShowInfo,   mShowInfo);
        settings.setValue(kSettingsFilterMode, static_cast<int32_t>(mFilteringMode));
        settings.setValue(kSettingsZoomMode,   static_cast<int32_t>(mZoomMode));
        settings.setValue(kSettingsToneMapping, static_cast<int32_t>(mImageProcessor->toneMappingMode()));

        if (mToolTip) {
            delete mToolTip;
        }
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
        const auto filteringGroup = new QActionGroup(this);
        ActionsArray<FilteringMode> filter = {};

        filter[FilteringMode::eNone] = createMenuAction("No filter");
        filter[FilteringMode::eNone]->setCheckable(true);
        filter[FilteringMode::eNone]->setActionGroup(filteringGroup);

        filter[FilteringMode::eAntialiasing] = createMenuAction("Antialiasing");
        filter[FilteringMode::eAntialiasing]->setCheckable(true);
        filter[FilteringMode::eAntialiasing]->setActionGroup(filteringGroup);

        filter[mFilteringMode]->setChecked(true);

        connect(filter[FilteringMode::eNone],         &QAction::triggered, this, &CanvasWidget::onActNoFilter);
        connect(filter[FilteringMode::eAntialiasing], &QAction::triggered, this, &CanvasWidget::onActAntialiasing);

        menu->addAction(filter[FilteringMode::eNone]);
        menu->addAction(filter[FilteringMode::eAntialiasing]);
        menu->addSeparator();
    }

    // Rotation
    {
        const auto & rotate = mActRotate.get(); 
        menu->addAction(rotate[Rotation::eDegree0]);
        menu->addAction(rotate[Rotation::eDegree90]);
        menu->addAction(rotate[Rotation::eDegree180]);
        menu->addAction(rotate[Rotation::eDegree270]);
        menu->addSeparator();
    }

    // Zoom
    {
        const auto & zoom = mActZoom.get(); 
        menu->addAction(zoom[ZoomMode::eFree]);
        menu->addAction(zoom[ZoomMode::eFitWindow]);
        menu->addAction(zoom[ZoomMode::eFixed]);
        menu->addSeparator();
    }

    // ToneMapping
    {
        // ToDo (.gruzdev): Temporal arrow fix
        const auto tmAction = createMenuAction(QString::fromUtf8("Tone mapping " "\xE2\x80\xA3"));
        QMenu* tmMenu = new QMenu(menu);
        if (mImage && testFlag(mImage->getFrame().flags, FrameFlags::eHRD)) {
            const auto & tmActions = mActToneMapping.get();

            tmMenu->addAction(tmActions[FIETMO_NONE]);
            tmMenu->addAction(tmActions[FIETMO_LINEAR]);
            tmMenu->addAction(tmActions[FIETMO_DRAGO03]);
            tmMenu->addAction(tmActions[FIETMO_REINHARD05]);
            tmMenu->addAction(tmActions[FIETMO_FATTAL02]);

            //tmActions[FIETMO_DRAGO03]->setEnabled(mImage->isRGB());
            //tmActions[FIETMO_REINHARD05]->setEnabled(mImage->isRGB());
            //tmActions[FIETMO_FATTAL02]->setEnabled(mImage->isRGB());

            tmAction->setEnabled(true);
        }
        else {
            tmAction->setEnabled(false);
        }
        tmAction->setMenu(tmMenu);
        menu->addAction(tmAction);
        menu->addSeparator();
    }

    const auto actQuit = createMenuAction("Quit");
    connect(actQuit, &QAction::triggered, this, &QWidget::close);
    menu->addAction(actQuit);

    return menu;
}

CanvasWidget::ActionsArray<Rotation> CanvasWidget::initRotationActions()
{
    const auto groupRotation = new QActionGroup(this);
    ActionsArray<Rotation> actions = {};

    actions[Rotation::eDegree0] = createMenuAction(QString::fromUtf8("Rotation 0" UTF8_DEGREE));
    actions[Rotation::eDegree0]->setCheckable(true);
    actions[Rotation::eDegree0]->setActionGroup(groupRotation);
    actions[Rotation::eDegree0]->setChecked(true);

    actions[Rotation::eDegree90] = createMenuAction(QString::fromUtf8("Rotation 90" UTF8_DEGREE));
    actions[Rotation::eDegree90]->setCheckable(true);
    actions[Rotation::eDegree90]->setActionGroup(groupRotation);

    actions[Rotation::eDegree180] = createMenuAction(QString::fromUtf8("Rotation 180" UTF8_DEGREE));
    actions[Rotation::eDegree180]->setCheckable(true);
    actions[Rotation::eDegree180]->setActionGroup(groupRotation);

    actions[Rotation::eDegree270] = createMenuAction(QString::fromUtf8("Rotation -90" UTF8_DEGREE));
    actions[Rotation::eDegree270]->setCheckable(true);
    actions[Rotation::eDegree270]->setActionGroup(groupRotation);

    connect(actions[Rotation::eDegree0],   &QAction::triggered, std::bind(&CanvasWidget::onActRotation, this, std::placeholders::_1, Rotation::eDegree0));
    connect(actions[Rotation::eDegree90],  &QAction::triggered, std::bind(&CanvasWidget::onActRotation, this, std::placeholders::_1, Rotation::eDegree90));
    connect(actions[Rotation::eDegree180], &QAction::triggered, std::bind(&CanvasWidget::onActRotation, this, std::placeholders::_1, Rotation::eDegree180));
    connect(actions[Rotation::eDegree270], &QAction::triggered, std::bind(&CanvasWidget::onActRotation, this, std::placeholders::_1, Rotation::eDegree270));

    return actions;
}

CanvasWidget::ActionsArray<ZoomMode> CanvasWidget::initZoomActions()
{
    const auto groupZoom = new QActionGroup(this);
    ActionsArray<ZoomMode> actions = {};

    actions[ZoomMode::eFree] = createMenuAction(QString::fromUtf8("Free zoom"));
    actions[ZoomMode::eFree]->setCheckable(true);
    actions[ZoomMode::eFree]->setActionGroup(groupZoom);

    actions[ZoomMode::eFitWindow] = createMenuAction(QString::fromUtf8("Fit window"));
    actions[ZoomMode::eFitWindow]->setCheckable(true);
    actions[ZoomMode::eFitWindow]->setActionGroup(groupZoom);

    actions[ZoomMode::eFixed] = createMenuAction(QString::fromUtf8("Fix zoom"));
    actions[ZoomMode::eFixed]->setCheckable(true);
    actions[ZoomMode::eFixed]->setActionGroup(groupZoom);

    actions[mZoomMode]->setChecked(true);

    connect(actions[ZoomMode::eFree],      &QAction::triggered, std::bind(&CanvasWidget::onActZoomMode, this, std::placeholders::_1, ZoomMode::eFree));
    connect(actions[ZoomMode::eFitWindow], &QAction::triggered, std::bind(&CanvasWidget::onActZoomMode, this, std::placeholders::_1, ZoomMode::eFitWindow));
    connect(actions[ZoomMode::eFixed],     &QAction::triggered, std::bind(&CanvasWidget::onActZoomMode, this, std::placeholders::_1, ZoomMode::eFixed));

    return actions;
}

CanvasWidget::TMActionsArray CanvasWidget::initToneMappingActions()
{
    const auto groupTM = new QActionGroup(this);
    TMActionsArray actions = {};

    actions[FIETMO_NONE] = createMenuAction(QString::fromUtf8(FreeImageExt_TMtoString(FIETMO_NONE)));
    actions[FIETMO_NONE]->setCheckable(true);
    actions[FIETMO_NONE]->setActionGroup(groupTM);

    actions[FIETMO_LINEAR] = createMenuAction(QString::fromUtf8(FreeImageExt_TMtoString(FIETMO_LINEAR)));
    actions[FIETMO_LINEAR]->setCheckable(true);
    actions[FIETMO_LINEAR]->setActionGroup(groupTM);

    actions[FIETMO_DRAGO03] = createMenuAction(QString::fromUtf8(FreeImageExt_TMtoString(FIETMO_DRAGO03)));
    actions[FIETMO_DRAGO03]->setCheckable(true);
    actions[FIETMO_DRAGO03]->setActionGroup(groupTM);

    actions[FIETMO_REINHARD05] = createMenuAction(QString::fromUtf8(FreeImageExt_TMtoString(FIETMO_REINHARD05)));
    actions[FIETMO_REINHARD05]->setCheckable(true);
    actions[FIETMO_REINHARD05]->setActionGroup(groupTM);

    actions[FIETMO_FATTAL02] = createMenuAction(QString::fromUtf8(FreeImageExt_TMtoString(FIETMO_FATTAL02)));
    actions[FIETMO_FATTAL02]->setCheckable(true);
    actions[FIETMO_FATTAL02]->setActionGroup(groupTM);

    actions[mImageProcessor->toneMappingMode()]->setChecked(true);

    connect(actions[FIETMO_NONE],       &QAction::triggered, std::bind(&CanvasWidget::onActToneMapping, this, std::placeholders::_1, FIETMO_NONE      ));
    connect(actions[FIETMO_LINEAR],     &QAction::triggered, std::bind(&CanvasWidget::onActToneMapping, this, std::placeholders::_1, FIETMO_LINEAR    ));
    connect(actions[FIETMO_DRAGO03],    &QAction::triggered, std::bind(&CanvasWidget::onActToneMapping, this, std::placeholders::_1, FIETMO_DRAGO03   ));
    connect(actions[FIETMO_REINHARD05], &QAction::triggered, std::bind(&CanvasWidget::onActToneMapping, this, std::placeholders::_1, FIETMO_REINHARD05));
    connect(actions[FIETMO_FATTAL02],   &QAction::triggered, std::bind(&CanvasWidget::onActToneMapping, this, std::placeholders::_1, FIETMO_FATTAL02  ));

    return actions;
}

void CanvasWidget::onShowContextMenu(const QPoint & p)
{
    try {
        // Deferred initialization
        if (!mContextMenu) {
            mContextMenu = createContextMenu();
        }
        if (mContextMenu) {
            mContextMenu->popup(mapToGlobal(p));
        }
    }
    catch(std::exception & err) {
        qWarning() << QString("CanvasWidget[onShowContextMenu]: ") + QString(err.what());
    }
}

void CanvasWidget::invalidateImageDescription()
{
    mInfoIsValid = false;
    update();
}

void CanvasWidget::onImageReady(ImagePtr image, size_t imgIdx, size_t imgCount)
{
    mImageProcessor->detachSource();
    if(mContextMenu) {
        delete mContextMenu;
        mContextMenu = nullptr;
    }
    mAnimIndex = kNoneIndex;

    mImage = std::move(image);
    if(mImage) {
        if(!mImageDescription) {
            mImageDescription = std::make_unique<ImageDescription>();
        }
        mImageDescription->setImageInfo(mImage->info());
        if (mImage && !mImage->isNull()) {
            const auto fitRect = fitWidth(mImage->width(), mImage->height());
            if(!mZoomController) {
                mZoomController = std::make_unique<ZoomController>(mImage->width(), fitRect.width());
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
                    mZoomController->rebase(mImage->width(), fitRect.width());
                }
                else {
                    mZoomController = std::make_unique<ZoomController>(mImage->width(), fitRect.width());
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

                mEnableAnimation = mImage->info().animated;
            }
            else {
                if (mPageText) {
                    delete mPageText;
                    mPageText = nullptr;
                }
                mEnableAnimation  = false;
            }

            mImageDescription->setZoom(mZoomController->getFactor());
            if (imgIdx < imgCount) {
                mImageDescription->setImageIndex(imgIdx, imgCount);
            }

            mImageProcessor->attachSource(mImage);
        }

        mImageDescription->setFormat(mImage->getFrame().srcFormat);

        setWindowTitle(mImage->info().path + " - " + QApplication::applicationName());
    }

    invalidateImageDescription();

    if(!isVisible()) {
        show();
    }
    mTransitionRequested = false;
    mErrorText->hide();
    update();
}

void CanvasWidget::onImageDirScanned(size_t imgIdx, size_t totalCount)
{
    if(!mImageDescription) {
        mImageDescription = std::make_unique<ImageDescription>();
    }
    if (imgIdx < totalCount) {
        mImageDescription->setImageIndex(imgIdx, totalCount);
    }
    else {
        mImageDescription->setImageIndex(0, 0);
    }
    if(mInfoText) {
        mInfoText->setText(mImageDescription->toLines());
    }
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
    switch(mImageProcessor->rotation()) {
    case Rotation::eDegree0:
    case Rotation::eDegree180:
        dw = mZoomController->getValue();
        dh = static_cast<int>(static_cast<int64_t>(dw) * h / w);
        break;
    case Rotation::eDegree90:
    case Rotation::eDegree270:
        dh = mZoomController->getValue();
        dw = static_cast<int>(static_cast<int64_t>(dh) * h / w);
        break;
    default:
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
    if (mZoomController && mImageDescription) {
        mImageDescription->setZoom(mZoomController->getFactor());
        invalidateImageDescription();
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

    if (mImage && !mImage->isNull()) {
        QPainter painter(this);
        if (mFilteringMode == FilteringMode::eAntialiasing) {
            painter.setRenderHint(QPainter::RenderHint::SmoothPixmapTransform, true);
        }

        const ImageFrame & frame = mImage->getFrame();

        mImageRect = calculateImageRegion();
        const auto dstCenter = mImageRect.center();

        // Make vertical flip
        const auto trans1 = QTransform::fromTranslate(-dstCenter.x(), -dstCenter.y());
        const auto scale  = QTransform::fromScale(1.0, -1.0);
        const auto trans2 = QTransform::fromTranslate(dstCenter.x(), dstCenter.y());
        painter.setTransform(trans1 * scale * trans2);

        painter.drawPixmap(mImageRect, mImageProcessor->getResult());

        if (mShowInfo) {
            if (!mInfoIsValid) {
                if (mInfoText && mImageDescription) {
                    mInfoText->setText(mImageDescription->toLines());
                }
                mInfoIsValid = true;
            }
            mInfoText->show();
            if (mPageText) {
                mPageText->setText("Page " + QString::number(frame.index + 1) + "/" + QString::number(mImage->pagesCount()));
                mPageText->show();
            }
        }
        else {
            mInfoText->hide();
            if (mPageText) {
                mPageText->hide();
            }
        }

        if (mEnableAnimation) {
            if (frame.index != mAnimIndex) {
                new UniqueTick(mImage->id(), frame.duration, this, &CanvasWidget::onAnimationTick, this);
            }
            mAnimIndex = frame.index;
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
        error.push_back(mImage ? mImage->info().path : "");
        mErrorText->setText(error);
        mErrorText->move(width() / 2 - mErrorText->width() / 2, height() / 2 - mErrorText->height() / 2);
        mErrorText->show();
    }

    if(mStartup){
#ifdef _MSC_VER
        std::cout << (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - mStartTime).count() / 1e3) << std::endl;
#endif
        mStartup = false;
    }
}

void CanvasWidget::recalculateFittingScale()
{
    if(mZoomController && mImage && !mImage->isNull()) {
        const auto r = fitWidth(mImage->width(), mImage->height());
        switch(mImageProcessor->rotation()) {
            case Rotation::eDegree0:
            case Rotation::eDegree180:
                mZoomController->setFitValue(r.width());
                break;
            case Rotation::eDegree90:
            case Rotation::eDegree270:
                mZoomController->setFitValue(r.height());
                break;
            default:
            break;
        }
        updateZoomLabel();
    }
}

void CanvasWidget::resizeEvent(QResizeEvent * event)
{
    QWidget::resizeEvent(event);
    if (mImage && !mImage->isNull()) {
        updateOffsets();
        recalculateFittingScale();
        if (mZoomController && (mZoomMode == ZoomMode::eFitWindow)) {
            mZoomController->moveToFit();
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
        
    }
    else if (QApplication::keyboardModifiers() == Qt::ControlModifier) {
        // Ctrl + Key
        if (event->key() == Qt::Key_R) {
            emit evenReloadImage();
        }
        else if (event->key() == Qt::Key_Up) {
            if(mImage && mImage->notNull()) {
                mActRotate.get()[Rotation::eDegree0]->trigger();
            }
        }
        else if (event->key() == Qt::Key_Left) {
            if(mImage && mImage->notNull()) {
                mActRotate.get()[Rotation::eDegree270]->trigger();
            }
        }
        else if (event->key() == Qt::Key_Right) {
            if(mImage && mImage->notNull()) {
                mActRotate.get()[Rotation::eDegree90]->trigger();
            }
        }
        else if (event->key() == Qt::Key_Down) {
            if(mImage && mImage->notNull()) {
                mActRotate.get()[Rotation::eDegree180]->trigger();
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
        else if (event->key() == Qt::Key_Asterisk) {
            if (mImage && mImage->notNull() && mZoomController) {
                if(mZoomMode != ZoomMode::eFitWindow) {
                    mActZoom.get()[ZoomMode::eFitWindow]->trigger();
                }
                else {
                    mZoomController->moveToIdentity();
                    mActZoom.get()[ZoomMode::eFree]->trigger();
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
        else if (event->key() == Qt::Key_Space) {
            if (mImage && mImage->notNull() && mImage->pagesCount() > 1) {
                if (mEnableAnimation) {
                    mEnableAnimation = false;
                }
                else {
                    mEnableAnimation = true;
                    mAnimIndex = -1;
                    update();
                }
            }
        }
        else if (event->key() == Qt::Key_PageUp) {
            if (mImage && mImage->notNull() && mImage->pagesCount() > 1 && !mEnableAnimation) {
                try {
                    mImage->next();
                    update();
                }
                catch(...)
                { }
            }
        }
        else if (event->key() == Qt::Key_PageDown) {
            if (mImage && mImage->notNull() && mImage->pagesCount() > 1 && !mEnableAnimation) {
                try {
                    mImage->prev();
                    update();
                }
                catch(...)
                { }
            }
        }
        else if(event->key() == Qt::Key_Alt || event->key() == Qt::Key_AltGr) {
            showTooltip(mCursorPosition);
        }
    }
}

void CanvasWidget::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Alt || event->key() == Qt::Key_AltGr) {
        hideTooltip();
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
    else if(event->button() & Qt::RightButton) {
        // drag image
        mBrowsing = true;
        mClickPos = event->pos();
        mMenuPos  = event->pos();
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
    if (mBrowsing && mMenuPos == event->pos()) {
        emit customContextMenuRequested(mMenuPos);
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
        update();
    }
}

void CanvasWidget::showTooltip(const QPoint & pos)
{
    unsetCursor();

    if (mImageProcessor && mImageRect.contains(pos)) {
        QPoint imgPos = pos - mImageRect.topLeft();

        // Invert zoom
        imgPos.setX(static_cast<int>(std::floor((imgPos.x() + 0.5) / mZoomController->getFactor())));
        imgPos.setY(static_cast<int>(std::floor((imgPos.y() + 0.5) / mZoomController->getFactor())));

        Pixel pixelValue{};
        if (mImageProcessor->getPixel(imgPos.y(), imgPos.x(), &pixelValue)) {
            if (!mToolTip) {
                mToolTip = new TextWidget(nullptr, Qt::black, 12);
                mToolTip->setWindowFlags(Qt::ToolTip);
                mToolTip->setBackgroundColor(QColor::fromRgb(255, 255, 225));
                mToolTip->setBorderColor(Qt::black);
                mToolTip->setPaddings(4, 2, 0, -4);
            }
            mToolTip->move(mapToGlobal(pos) + QPoint(7, 20));
            mToolTip->setText({ QString("Y: %1, X: %2").arg(pixelValue.y).arg(pixelValue.x), pixelValue.repr });
            mToolTip->setColor(Qt::black);
            mToolTip->show();
            mToolTip->update();
        }
    }
    else {
        hideTooltip();
    }
}

void CanvasWidget::hideTooltip()
{
    if (mToolTip) {
        // ToDo (a.gruzdev): Workaround to hide previous content
        mToolTip->setColor(Qt::transparent);
        mToolTip->repaint();
        mToolTip->hide();
    }
}

void CanvasWidget::mouseMoveEvent(QMouseEvent* event)
{
    //QWidget::mouseMoveEvent(event);

    if (QApplication::keyboardModifiers() == Qt::AltModifier) {
        // Alt pressed -> pixel view mode
        showTooltip(event->pos());
    }
    else {
        // Window controls
        hideTooltip();

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
        else if(!mFullScreen) {
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
            mActZoom.get()[ZoomMode::eFree]->trigger();
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
        const auto oldRot = mImageProcessor->rotation();
        if(oldRot != rot) {
            mImageProcessor->setRotation(rot);

            recalculateFittingScale();

            const auto dr = toDegree(rot) - toDegree(oldRot);
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
    if (mImage && mImage->id() == imgId && mImage->notNull() && mEnableAnimation) {
        try {
            mImage->next();
        }
        catch(...) {
            // ToDo (a.gruzdev): Report error here
        }
        update();
    }
}

void CanvasWidget::onActToneMapping(bool checked, FIE_ToneMapping m)
{
    if (checked && mImage && !mImage->isNull() && testFlag(mImage->getFrame().flags, FrameFlags::eHRD)) {
        mImageProcessor->setToneMappingMode(m);
        if (mImageDescription) {
            mImageDescription->setToneMapping(m);
        }
        invalidateImageDescription();
        update();
    }
}

