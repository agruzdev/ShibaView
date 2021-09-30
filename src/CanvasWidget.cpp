/**
 * @file
 *
 * Copyright 2018-2021 Alexey Gruzdev
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
#include <QActionGroup>
#include <QColor>
#include <QFileDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QSettings>
#include <QScreen>
#include <QToolTip>
#include <QTransform>
#include <QGraphicsDropShadowEffect>
#include <QRawFont>
#include <QWidgetAction>

#include <QKeySequence>

#include "AboutWidget.h"
#include "Controls.h"
#include "ExifWidget.h"
#include "Global.h"
#include "ImageProcessor.h"
#include "MenuWidget.h"
#include "TextWidget.h"
#include "Tooltip.h"
#include "ZoomController.h"
#include "UniqueTick.h"

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

    const QString kSettingsBackground  = "canvas/background";
    const QString kSettingsGeometry    = "canvas/geometry";
    const QString kSettingsFullscreen  = "canvas/fullscreen";
    const QString kSettingsShowInfo    = "canvas/info";
    const QString kSettingsZoomMode    = "canvas/zoom";
    const QString kSettingsRememberZoom = "canvas/remember_zoom";
    const QString kSettingsZoomScaleValue = "canvas/zoom_scale";
    const QString kSettingsZoomFitValue   = "canvas/zoom_fit";
    const QString kSettingsFilterMode  = "canvas/filtering";
    const QString kSettingsToneMapping = "canvas/tonemapping";
    const QString kSettingsCheckboard  = "canvas/checkboard";

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


    FilteringMode toFilteringMode(int32_t v)
    {
        switch(v) {
        case static_cast<int32_t>(FilteringMode::eAntialiasing):
            return FilteringMode::eAntialiasing;
        default:
        case static_cast<int32_t>(FilteringMode::eNone):
            return FilteringMode::eNone;
        }
    }

    ZoomMode toZoomMode(int32_t v)
    {
        switch(v) {
        case static_cast<int32_t>(ZoomMode::eIdentity):
            return ZoomMode::eIdentity;
        case static_cast<int32_t>(ZoomMode::eFixed):
            return ZoomMode::eFixed;
        default:
        case static_cast<int32_t>(ZoomMode::eFitWindow):
            return ZoomMode::eFitWindow;
        }
    }

}


CanvasWidget::CanvasWidget(std::chrono::steady_clock::time_point t)
    : QWidget(nullptr)
    , mBackgroundColor(QColor(0x2B, 0x2B, 0x2B))
    , mHoveredBorder(BorderPosition::eNone)
    , mStartTime(t)
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::MSWindowsOwnDC);

    mInfoText = new TextWidget(this);
    mInfoText->move(kTextPaddingLeft, kTextPaddingTop);
    mInfoText->enableShadow();

    mErrorText = new TextWidget(this);

    const auto kDefaultGeometry = QRect(200, 200, 1280, 720);

    QSettings settings;
    mBackgroundColor.setNamedColor(settings.value(kSettingsBackground, mBackgroundColor.name(QColor::NameFormat::HexRgb)).toString());
    mClickGeometry = settings.value(kSettingsGeometry, kDefaultGeometry).toRect();
    mFullScreen    = settings.value(kSettingsFullscreen, false).toBool();
    mShowInfo      = settings.value(kSettingsShowInfo, false).toBool();
    mFilteringMode = toFilteringMode(settings.value(kSettingsFilterMode, static_cast<int32_t>(FilteringMode::eNone)).toInt());
    mRememberZoom  = settings.value(kSettingsRememberZoom, false).toBool();
    if (mRememberZoom) {
        mZoomMode = toZoomMode(settings.value(kSettingsZoomMode, static_cast<int32_t>(ZoomMode::eFitWindow)).toInt());
    }
    else {
        mZoomMode = ZoomMode::eFitWindow;
    }

    QPalette palette;
    palette.setColor(QPalette::ColorRole::Window, mBackgroundColor);
    setPalette(palette);

    setMouseTracking(true);

    if (mFullScreen) {
        mFullScreen = setFullscreenGeometry();
    }
    if (!mFullScreen) {
        setGeometry(mClickGeometry);
    }

    mActRotate      = std::async(std::launch::deferred, [this]{ return initRotationActions();    });
    mActFlip        = std::async(std::launch::deferred, [this]{ return initFlipActions();        });
    mActZoom        = std::async(std::launch::deferred, [this]{ return initZoomActions();        });
    mActSwizzle     = std::async(std::launch::deferred, [this]{ return initSwizzleActions();     });
    mActGammaType   = std::async(std::launch::deferred, [this]{ return initGammaTypeActions();   });
    mActToneMapping = std::async(std::launch::deferred, [this]{ return initToneMappingActions(); });

    connect(this, &QWidget::customContextMenuRequested, this, &CanvasWidget::onShowContextMenu);

    mImageProcessor = std::make_unique<ImageProcessor>();
    mImageProcessor->setToneMappingMode(static_cast<FIE_ToneMapping>(settings.value(kSettingsToneMapping, static_cast<int32_t>(FIE_ToneMapping::FIETMO_NONE)).toInt()));

    mZoomController = std::make_unique<ZoomController>(16, settings.value(kSettingsZoomFitValue, 128).toInt(), settings.value(kSettingsZoomScaleValue, 0).toInt());

    connect(static_cast<QApplication*>(QApplication::instance()), &QApplication::applicationStateChanged, this, &CanvasWidget::applicationStateChanged);

    mShowTransparencyCheckboard = settings.value(kSettingsCheckboard, mShowTransparencyCheckboard).toBool();
    mCheckboard = std::async(std::launch::deferred, [this]{
        QPixmap p(16, 16);
        QPainter painter(&p);
        painter.fillRect(0, 0, 8, 8, QColor(Qt::lightGray));
        painter.fillRect(8, 0, 8, 8, QColor(Qt::white));
        painter.fillRect(8, 8, 8, 8, QColor(Qt::lightGray));
        painter.fillRect(0, 8, 8, 8, QColor(Qt::white));
        return p;
    });
}

CanvasWidget::~CanvasWidget()
{
    assert(mImageProcessor); // never null
    try {
        QSettings settings;
        settings.setValue(kSettingsBackground, mBackgroundColor.name(QColor::NameFormat::HexRgb));
        settings.setValue(kSettingsGeometry,   mClickGeometry);
        settings.setValue(kSettingsFullscreen, mFullScreen);
        settings.setValue(kSettingsShowInfo,   mShowInfo);
        settings.setValue(kSettingsFilterMode, static_cast<int32_t>(mFilteringMode));
        settings.setValue(kSettingsRememberZoom, mRememberZoom);
        if (mRememberZoom) {
            settings.setValue(kSettingsZoomMode, static_cast<int32_t>(mZoomMode));
        }
        settings.setValue(kSettingsZoomScaleValue, static_cast<int32_t>(mZoomController->getScaleValue()));
        settings.setValue(kSettingsZoomFitValue,   static_cast<int32_t>(mZoomController->getFitValue()));
        settings.setValue(kSettingsToneMapping,    static_cast<int32_t>(mImageProcessor->toneMappingMode()));
        settings.setValue(kSettingsCheckboard,     mShowTransparencyCheckboard);

        mTooltip.reset();
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
        const auto& rotate = mActRotate.get(); 
        menu->addAction(rotate[Rotation::eDegree0]);
        menu->addAction(rotate[Rotation::eDegree90]);
        menu->addAction(rotate[Rotation::eDegree180]);
        menu->addAction(rotate[Rotation::eDegree270]);
        menu->addSeparator();
    }

    // Flip
    {
        const auto& flip = mActFlip.get();
        menu->addAction(flip[FlipType::eHorizontal]);
        menu->addAction(flip[FlipType::eVertical]);
        menu->addSeparator();
    }

    // Zoom
    {
        const auto& zoom = mActZoom.get();
        zoom[mZoomMode]->setChecked(true);
        menu->addAction(zoom[ZoomMode::eIdentity]);
        menu->addAction(zoom[ZoomMode::eFitWindow]);
        menu->addAction(zoom[ZoomMode::eFixed]);

        const auto rememberZoomAct = createMenuAction(QString::fromUtf8("Freeze zoom mode"));
        rememberZoomAct->setCheckable(true);
        rememberZoomAct->setChecked(mRememberZoom);
        connect(rememberZoomAct, &QAction::triggered, this, &CanvasWidget::onActRememberZoom);
        menu->addAction(rememberZoomAct);

        menu->addSeparator();
    }

    // ToneMapping
    {
        // ToDo (.gruzdev): Temporal arrow fix
        const auto tmAction = createMenuAction(QString::fromUtf8("Tone mapping " "\xE2\x80\xA3"));
        QMenu* tmMenu = new QMenu(menu);
        if (mImage && mImage->notNull() && testFlag(mImage->getFrame().flags, FrameFlags::eHRD)) {
            auto& tmActions = mActToneMapping.get();
            // Manual order is important
            tmMenu->addAction(tmActions[FIETMO_NONE]);
            tmMenu->addAction(tmActions[FIETMO_LINEAR]);
            tmMenu->addAction(tmActions[FIETMO_DRAGO03]);
            tmMenu->addAction(tmActions[FIETMO_REINHARD05]);
            tmMenu->addAction(tmActions[FIETMO_FATTAL02]);
            for (int32_t i = 0; i < tmActions.size(); ++i) {
                tmActions[i]->setChecked(static_cast<FIE_ToneMapping>(i) == mImageProcessor->toneMappingMode());
            }
            tmAction->setEnabled(true);
        }
        else {
            tmAction->setEnabled(false);
        }
        tmAction->setMenu(tmMenu);
        menu->addAction(tmAction);
        menu->addSeparator();
    }

    // Gamma
    {
        const auto& gamma = mActGammaType.get();
        gamma[mGammaType]->setChecked(true);
        menu->addAction(gamma[GammaType::eLinear]);
        menu->addAction(gamma[GammaType::eGamma22]);
        menu->addAction(gamma[GammaType::eDegamma22]);
        menu->addSeparator();
    }

    // Swap channels
    {
        // ToDo (.gruzdev): Temporal arrow fix
        const auto swAction = createMenuAction(QString::fromUtf8("Channels " "\xE2\x80\xA3"));
        swAction->setEnabled(false);
        QMenu* swMenu = new QMenu(menu);
        if (mImage && mImage->notNull() && mImageProcessor && testFlag(mImage->getFrame().flags, FrameFlags::eRGB)) {
            const auto channelsNumber = mImage->channels();
            if (channelsNumber > 1) {
                auto& swActions = mActSwizzle.get();
                for (int32_t i = 0; i < swActions.size(); ++i) {
                    swActions[i]->setChecked(static_cast<ChannelSwizzle>(i) == mImageProcessor->getChannelSwizzle());
                    swMenu->addAction(swActions[i]);
                }
                swActions[ChannelSwizzle::eAlpha]->setEnabled(channelsNumber == 4);
                swAction->setEnabled(true);
            }
        }
        swAction->setMenu(swMenu);
        menu->addAction(swAction);
        menu->addSeparator();
    }

    const auto actTransparency = createMenuAction("Transparency");
    actTransparency->setCheckable(true);
    actTransparency->setChecked(mShowTransparencyCheckboard);
    connect(actTransparency, &QAction::triggered, this, &CanvasWidget::onActTransparency);
    menu->addAction(actTransparency);
    menu->addSeparator();

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

CanvasWidget::ActionsArray<FlipType> CanvasWidget::initFlipActions()
{
    ActionsArray<FlipType> actions = {};
    
    actions[FlipType::eHorizontal] = createMenuAction(QString::fromUtf8("Flip horizontally"));
    actions[FlipType::eHorizontal]->setCheckable(true);
    actions[FlipType::eHorizontal]->setChecked(false);

    actions[FlipType::eVertical] = createMenuAction(QString::fromUtf8("Flip vertically"));
    actions[FlipType::eVertical]->setCheckable(true);
    actions[FlipType::eVertical]->setChecked(false);

    connect(actions[FlipType::eHorizontal], &QAction::triggered, std::bind(&CanvasWidget::onActFlip, this, std::placeholders::_1, FlipType::eHorizontal));
    connect(actions[FlipType::eVertical],   &QAction::triggered, std::bind(&CanvasWidget::onActFlip, this, std::placeholders::_1, FlipType::eVertical));

    return actions;
}

CanvasWidget::ActionsArray<ZoomMode> CanvasWidget::initZoomActions()
{
    const auto groupZoom = new QActionGroup(this);
    ActionsArray<ZoomMode> actions = {};

    actions[ZoomMode::eIdentity] = createMenuAction(QString::fromUtf8("100 percents"));
    actions[ZoomMode::eIdentity]->setCheckable(true);
    actions[ZoomMode::eIdentity]->setActionGroup(groupZoom);

    actions[ZoomMode::eFitWindow] = createMenuAction(QString::fromUtf8("Fit window"));
    actions[ZoomMode::eFitWindow]->setCheckable(true);
    actions[ZoomMode::eFitWindow]->setActionGroup(groupZoom);

    actions[ZoomMode::eFixed] = createMenuAction(QString::fromUtf8("Fixed zoom"));
    actions[ZoomMode::eFixed]->setCheckable(true);
    actions[ZoomMode::eFixed]->setActionGroup(groupZoom);

    connect(actions[ZoomMode::eIdentity],  &QAction::triggered, std::bind(&CanvasWidget::onActZoomMode, this, std::placeholders::_1, ZoomMode::eIdentity));
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

    connect(actions[FIETMO_NONE],       &QAction::triggered, std::bind(&CanvasWidget::onActToneMapping, this, std::placeholders::_1, FIETMO_NONE      ));
    connect(actions[FIETMO_LINEAR],     &QAction::triggered, std::bind(&CanvasWidget::onActToneMapping, this, std::placeholders::_1, FIETMO_LINEAR    ));
    connect(actions[FIETMO_DRAGO03],    &QAction::triggered, std::bind(&CanvasWidget::onActToneMapping, this, std::placeholders::_1, FIETMO_DRAGO03   ));
    connect(actions[FIETMO_REINHARD05], &QAction::triggered, std::bind(&CanvasWidget::onActToneMapping, this, std::placeholders::_1, FIETMO_REINHARD05));
    connect(actions[FIETMO_FATTAL02],   &QAction::triggered, std::bind(&CanvasWidget::onActToneMapping, this, std::placeholders::_1, FIETMO_FATTAL02  ));

    return actions;
}

CanvasWidget::ActionsArray<GammaType> CanvasWidget::initGammaTypeActions()
{
    const auto group = new QActionGroup(this);
    ActionsArray<GammaType> actions = {};

    actions[GammaType::eLinear] = createMenuAction("Linear");
    actions[GammaType::eLinear]->setCheckable(true);
    actions[GammaType::eLinear]->setActionGroup(group);

    actions[GammaType::eGamma22] = createMenuAction("Gamma 1 / 2.2");
    actions[GammaType::eGamma22]->setCheckable(true);
    actions[GammaType::eGamma22]->setActionGroup(group);

    actions[GammaType::eDegamma22] = createMenuAction("Gamma 2.2");
    actions[GammaType::eDegamma22]->setCheckable(true);
    actions[GammaType::eDegamma22]->setActionGroup(group);

    connect(actions[GammaType::eLinear],    &QAction::triggered, std::bind(&CanvasWidget::onActGammaType, this, std::placeholders::_1, GammaType::eLinear));
    connect(actions[GammaType::eGamma22],   &QAction::triggered, std::bind(&CanvasWidget::onActGammaType, this, std::placeholders::_1, GammaType::eGamma22));
    connect(actions[GammaType::eDegamma22], &QAction::triggered, std::bind(&CanvasWidget::onActGammaType, this, std::placeholders::_1, GammaType::eDegamma22));

    return actions;
}

CanvasWidget::ActionsArray<ChannelSwizzle> CanvasWidget::initSwizzleActions()
{
    const auto group = new QActionGroup(this);
    ActionsArray<ChannelSwizzle> actions = {};

    actions[ChannelSwizzle::eRGB] = createMenuAction(QString::fromUtf8("RGB"));
    actions[ChannelSwizzle::eRGB]->setCheckable(true);
    actions[ChannelSwizzle::eRGB]->setActionGroup(group);

    actions[ChannelSwizzle::eBGR] = createMenuAction(QString::fromUtf8("BGR"));
    actions[ChannelSwizzle::eBGR]->setCheckable(true);
    actions[ChannelSwizzle::eBGR]->setActionGroup(group);

    actions[ChannelSwizzle::eRed] = createMenuAction(QString::fromUtf8("Red"));
    actions[ChannelSwizzle::eRed]->setCheckable(true);
    actions[ChannelSwizzle::eRed]->setActionGroup(group);

    actions[ChannelSwizzle::eGreen] = createMenuAction(QString::fromUtf8("Green"));
    actions[ChannelSwizzle::eGreen]->setCheckable(true);
    actions[ChannelSwizzle::eGreen]->setActionGroup(group);

    actions[ChannelSwizzle::eBlue] = createMenuAction(QString::fromUtf8("Blue"));
    actions[ChannelSwizzle::eBlue]->setCheckable(true);
    actions[ChannelSwizzle::eBlue]->setActionGroup(group);

    actions[ChannelSwizzle::eAlpha] = createMenuAction(QString::fromUtf8("Alpha"));
    actions[ChannelSwizzle::eAlpha]->setCheckable(true);
    actions[ChannelSwizzle::eAlpha]->setActionGroup(group);

    connect(actions[ChannelSwizzle::eRGB],   &QAction::triggered, std::bind(&CanvasWidget::onActSwizzle, this, std::placeholders::_1, ChannelSwizzle::eRGB));
    connect(actions[ChannelSwizzle::eBGR],   &QAction::triggered, std::bind(&CanvasWidget::onActSwizzle, this, std::placeholders::_1, ChannelSwizzle::eBGR));
    connect(actions[ChannelSwizzle::eRed],   &QAction::triggered, std::bind(&CanvasWidget::onActSwizzle, this, std::placeholders::_1, ChannelSwizzle::eRed));
    connect(actions[ChannelSwizzle::eGreen], &QAction::triggered, std::bind(&CanvasWidget::onActSwizzle, this, std::placeholders::_1, ChannelSwizzle::eGreen));
    connect(actions[ChannelSwizzle::eBlue],  &QAction::triggered, std::bind(&CanvasWidget::onActSwizzle, this, std::placeholders::_1, ChannelSwizzle::eBlue));
    connect(actions[ChannelSwizzle::eAlpha], &QAction::triggered, std::bind(&CanvasWidget::onActSwizzle, this, std::placeholders::_1, ChannelSwizzle::eAlpha));

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

            if (!mRememberZoom && !mTransitionIsReload) {
                if (width() >= static_cast<int64_t>(mImage->width()) && height() >= static_cast<int64_t>(mImage->height())) {
                    mZoomMode = ZoomMode::eIdentity;
                }
                else {
                    mZoomMode = ZoomMode::eFitWindow;
                }
            }

            switch (mZoomMode) {
            case ZoomMode::eIdentity:
                mZoomController->rebase(mImage->width(), fitRect.width());
                mZoomController->moveToIdentity();
                break;
            case ZoomMode::eFixed:
                mZoomController->rebase(mImage->width());
                break;
            default:
            case ZoomMode::eFitWindow:
                mZoomController->rebase(mImage->width(), fitRect.width());
                mZoomController->moveToFit();
                resetOffsets();
                break;
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
            mImageDescription->setFormat(mImage->getFrame().page->describeFormat());
        }

        setWindowTitle(mImage->info().path + " - " + QApplication::applicationName());
    }

    invalidateImageDescription();
    invalidateTooltip();
    invalidateExif();

    if(!isVisible()) {
        show();
    }
    mTransitionRequested = false;
    mTransitionIsReload  = false;
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
    const int windowWidth  = width();
    const int windowHeight = height();

    const int zeroX = windowWidth  / 2;
    const int zeroY = windowHeight / 2;

    const float kx = static_cast<float>(windowWidth)  / w;
    const float ky = static_cast<float>(windowHeight) / h;

    int fitWidth  = 1;
    int fitHeight = 1;
    if (kx < ky) {
        fitWidth  = windowWidth;
        fitHeight = static_cast<int>(static_cast<int64_t>(fitWidth) * h / w);
    }
    else {
        fitHeight = windowHeight;
        fitWidth  = static_cast<int>(static_cast<int64_t>(fitHeight) * w / h);
    }

    QRect r;
    r.setLeft(zeroX - fitWidth  / 2);
    r.setTop (zeroY - fitHeight / 2);
    r.setRight(zeroX + (fitWidth - fitWidth / 2));
    r.setBottom(zeroY + (fitHeight - fitHeight / 2));

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

    bool success = false;
    try {
        if (mImage && !mImage->isNull()) {
            QPainter painter(this);
            if (mFilteringMode == FilteringMode::eAntialiasing) {
                painter.setRenderHint(QPainter::RenderHint::SmoothPixmapTransform, true);
            }

            const ImageFrame & frame = mImage->getFrame();

            const auto imageRect = calculateImageRegion();
            const auto dstCenter = QRectF(imageRect).center();

            // Make vertical flip
            const auto trans1 = QTransform::fromTranslate(-dstCenter.x(), -dstCenter.y());
            const auto scale  = QTransform::fromScale(1.0, -1.0);
            const auto trans2 = QTransform::fromTranslate(dstCenter.x(), dstCenter.y());
            painter.setTransform(trans1 * scale * trans2);

            if (mShowTransparencyCheckboard) {
                painter.drawTiledPixmap(imageRect, mCheckboard.get());
            }
            painter.drawPixmap(imageRect, mImageProcessor->getResultPixmap());

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

            success = true;
        }
    }
    catch(const std::exception& err) {
        qDebug() << err.what();
    }
    catch(...) {
    }

    if (success) {
        mErrorText->hide();
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

    invalidateTooltip();

    if (mStartup) {
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
    switch(Controls::getInstance().decodeAction(event)) {

    case ControlAction::eOverlay:
        mShowInfo = !mShowInfo;
        update();
        break;

    case ControlAction::eSwitchZoom:
        if (mImage && mImage->notNull() && mZoomController) {
            if (mZoomMode != ZoomMode::eFitWindow) {
                mActZoom.get()[ZoomMode::eFitWindow]->trigger();
            }
            else {
                mZoomController->moveToIdentity();
                mActZoom.get()[ZoomMode::eIdentity]->trigger();
            }
            updateZoomLabel();
            resetOffsets();
            update();
        }
        break;

    case ControlAction::eReload:
        if (!mTransitionRequested) {
            emit eventReloadImage();
            mTransitionRequested = true;
            mTransitionIsReload = true;
        }
        break;

    case ControlAction::eOpenFile:
        if (!mTransitionRequested) {
            emit eventOpenImage();
            mTransitionRequested = true;
        }
        break;

    case ControlAction::eSaveFile:
        if (!mTransitionRequested && !mEnableAnimation) {
            if (mImage && mImage->notNull() && mImageProcessor) {
                QString error;
                try {
                    QDir imageDir;
                    if (!mImage->info().path.isEmpty()) {
                        imageDir = QFileInfo(mImage->info().path).dir();
                    }
                    const QString filename = QFileDialog::getSaveFileName(this, tr("Save file"), imageDir.filePath("Untitled.png"), tr("Images (*.png *.jpg *.bmp)"));
                    if (!filename.isEmpty()) {
                        const auto& bitmap = mImageProcessor->getResultBitmap();
                        ImageSource::Save(bitmap.get(), filename);
                    }
                }
                catch(std::exception& err) {
                    error = QString::fromUtf8(err.what());
                }
                if (!error.isEmpty()) {
                    QMessageBox::critical(this, "Error!", tr("Failed to save file. Reason: ") + QString(error));
                }
            }
        }
        break;

    case ControlAction::eRotation0:
        if (mImage && mImage->notNull()) {
            mActRotate.get()[Rotation::eDegree0]->trigger();
        }
        break;

    case ControlAction::eRotation270:
        if (mImage && mImage->notNull()) {
            mActRotate.get()[Rotation::eDegree270]->trigger();
        }
        break;

    case ControlAction::eRotation90:
        if(mImage && mImage->notNull()) {
            mActRotate.get()[Rotation::eDegree90]->trigger();
        }
        break;

    case ControlAction::eRotation180:
        if (mImage && mImage->notNull()) {
            mActRotate.get()[Rotation::eDegree180]->trigger();
        }
        break;

    case ControlAction::eColorPicker:
        if (!mTooltip) {
            mTooltip = std::make_unique<Tooltip>();
            mTooltip->hide();
            invalidateTooltip();
        }
        else {
            mTooltip.reset();
        }
        break;

    case ControlAction::eZoomIn:
        zoomToTarget(QPoint(width() / 2, height() / 2), 1);
        break;

    case ControlAction::eZoomOut:
        zoomToTarget(QPoint(width() / 2, height() / 2), -1);
        break;

    case ControlAction::ePreviousImage:
        if (!mTransitionRequested) {
            emit eventPrevImage();
            mTransitionRequested = true;
        }
        break;
        
    case ControlAction::eNextImage:
        if (!mTransitionRequested) {
            emit eventNextImage();
            mTransitionRequested = true;
        }
        break;
        
    case ControlAction::eFirstImage:
        if (!mTransitionRequested) {
            emit eventFirstImage();
            mTransitionRequested = true;
        }
        break;

    case ControlAction::eLastImage:
        if (!mTransitionRequested) {
            emit eventLastImage();
            mTransitionRequested = true;
        }
        break;

    case ControlAction::ePause:
        if (mImage && mImage->notNull() && mImage->pagesCount() > 1) {
            if (mEnableAnimation) {
                mEnableAnimation = false;
            }
            else {
                mEnableAnimation = true;
                mAnimIndex = kNoneIndex;
                update();
            }
        }
        break;

    case ControlAction::ePreviousFrame:
        if (mImage && mImage->notNull() && mImage->pagesCount() > 1 && !mEnableAnimation) {
            try {
                mImage->next();
                update();
            }
            catch(...)
            { }
        }
        break;

    case ControlAction::eNextFrame:
        if (mImage && mImage->notNull() && mImage->pagesCount() > 1 && !mEnableAnimation) {
            try {
                mImage->prev();
                update();
            }
            catch(...)
            { }
        }
        break;

    case ControlAction::eAbout:
        AboutWidget::getInstance().popUp();
        break;

    case ControlAction::eImageInfo:
        ExifWidget::getInstance().activate();
        invalidateExif();
        break;

    case ControlAction::eQuit:
        close();
        break;

    default:
        break;
    }
}

void CanvasWidget::keyReleaseEvent(QKeyEvent* /*event*/)
{
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
        mClickPos = event->globalPosition().toPoint();
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

bool CanvasWidget::setFullscreenGeometry()
{
#ifdef _WIN32
    const auto getScreenGeometry_ = [](const QScreen& s) { return s.geometry(); };
#else
    const auto getScreenGeometry_ = [](const QScreen& s) { return s.availableGeometry(); };
#endif
    if (const auto screen = QApplication::screenAt(mClickGeometry.center())) {
        setGeometry(getScreenGeometry_(*screen));
        return true;
    }
    if (const auto screen = QApplication::primaryScreen()) {
        setGeometry(getScreenGeometry_(*screen));
        return true;
    }
    return false;
}

void CanvasWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    QWidget::mouseDoubleClickEvent(event);
    if((event->button() & Qt::LeftButton) && (mHoveredBorder == BorderPosition::eNone)) {
        if (mFullScreen) {
            setGeometry(mClickGeometry);
            mFullScreen = false;
        } else {
            const auto currentGeomentry = geometry();
            if (setFullscreenGeometry()) {
                mClickGeometry = currentGeomentry;
                mFullScreen = true;
            }
        }
        update();
    }
}

void CanvasWidget::applicationStateChanged(Qt::ApplicationState state)
{
    mCursorPosition = mapFromGlobal(QCursor::pos());
    if (mTooltip) {
        if (state == Qt::ApplicationState::ApplicationActive) {
            invalidateTooltip();
        }
        else {
            mTooltip->hide();
        }
    }
}

void CanvasWidget::mouseMoveEvent(QMouseEvent* event)
{
    //QWidget::mouseMoveEvent(event);
    if (event) {

        mCursorPosition = event->pos();

        // Window controls
        if (!mFullScreen && mDragging) {
            const auto globalPos = event->globalPosition().toPoint();
            move(globalPos.x() - mClickPos.x(), globalPos.y() - mClickPos.y());
        }
        else if (mBrowsing) {
            mOffset += event->pos() - mClickPos;
            mClickPos = event->pos();
            updateOffsets();
            repaint();
        }
        else if (!mFullScreen && !mTooltip) {
            if (mStretching) {
                QRect r = mClickGeometry;
                if ((mHoveredBorder & BorderPosition::eLeft) != BorderPosition::eNone) {
                    const auto globalPos = event->globalPosition().toPoint();
                    r.setX(std::min(r.x() + globalPos.x() - mClickPos.x(), r.right() - kMinSize));
                }
                if ((mHoveredBorder & BorderPosition::eRight) != BorderPosition::eNone) {
                    const auto pos = event->position().toPoint();
                    r.setWidth(std::max(kMinSize, pos.x()));
                }
                if ((mHoveredBorder & BorderPosition::eTop) != BorderPosition::eNone) {
                    const auto globalPos = event->globalPosition().toPoint();
                    r.setY(std::min(r.y() + globalPos.y() - mClickPos.y(), r.bottom() - kMinSize));
                }
                if ((mHoveredBorder & BorderPosition::eBottom) != BorderPosition::eNone) {
                    const auto pos = event->position().toPoint();
                    r.setHeight(std::max(kMinSize, pos.y()));
                }
                setGeometry(r);
                updateOffsets();
            }
            else {
                BorderPosition borderPos = BorderPosition::eNone;
                if (mCursorPosition.x() <= kFrameThickness) {
                    borderPos = borderPos | BorderPosition::eLeft;
                }
                if (width() - mCursorPosition.x() <= kFrameThickness) {
                    borderPos = borderPos | BorderPosition::eRight;
                }
                if (mCursorPosition.y() <= kFrameThickness) {
                    borderPos = borderPos | BorderPosition::eTop;
                }
                if (height() - mCursorPosition.y() <= kFrameThickness) {
                    borderPos = borderPos | BorderPosition::eBottom;
                }
                switch (borderPos) {
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
                mHoveredBorder = borderPos;
            }
        }

        invalidateTooltip();
    }
}

void CanvasWidget::leaveEvent(QEvent* event)
{
    if (mTooltip) {
        mTooltip->hide();
    }
}

void CanvasWidget::invalidateTooltip()
{
    if (mTooltip) {
        const auto imageRect = calculateImageRegion();
        if (mImageProcessor && imageRect.contains(mCursorPosition)) {
            unsetCursor();

            QPoint imgPos = mCursorPosition - imageRect.topLeft();

            // Invert zoom
            imgPos.setX(static_cast<int>(std::floor((imgPos.x() + 0.5) / mZoomController->getFactor())));
            imgPos.setY(static_cast<int>(std::floor((imgPos.y() + 0.5) / mZoomController->getFactor())));

            Pixel pixelValue{};
            if (mImageProcessor->getPixel(imgPos.y(), imgPos.x(), &pixelValue)) {
                mTooltip->move(mapToGlobal(mCursorPosition));
                mTooltip->setText({ QString("Y: %1, X: %2").arg(pixelValue.y).arg(pixelValue.x), pixelValue.repr });
                mTooltip->show();
            }
            else {
                mTooltip->hide();
            }
        }
        else {
            mTooltip->hide();
        }
    }
}

void CanvasWidget::invalidateExif()
{
    ExifWidget& exifWidget = ExifWidget::getInstance();
    if (exifWidget.isActive()) {
        if (mImage && mImage->notNull()) {
            exifWidget.setExif(mImage->getFrame().page->getExif());
        }
        else {
            exifWidget.setEmpty();
        }
    }
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

        if (mZoomMode != ZoomMode::eFixed) {
            mActZoom.get()[ZoomMode::eFixed]->trigger();
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
            mCursorPosition = event->position().toPoint();
            zoomToTarget(mCursorPosition, (degrees.y() > 0) ? 1 : -1);
            invalidateTooltip();
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

void CanvasWidget::onActFlip(bool checked, FlipType f)
{
    if (mImageProcessor) {
        mImageProcessor->setFlip(f, checked);
        update();
    }
}

void CanvasWidget::onActZoomMode(bool checked, ZoomMode z)
{
    if (checked && mZoomController) {
        mZoomMode = z;
        switch(mZoomMode) {
        case ZoomMode::eIdentity:
            mZoomController->moveToIdentity();
            updateZoomLabel();
            updateOffsets();
            update();
            break;

        case ZoomMode::eFitWindow:
            mZoomController->moveToFit();
            updateZoomLabel();
            updateOffsets();
            update();
            break;

        default:
        case ZoomMode::eFixed:
            break;
        }
    }
}

void CanvasWidget::onActRememberZoom(bool checked)
{
    if (checked != mRememberZoom) {
        mRememberZoom = checked;
        QSettings settings;
        settings.setValue(kSettingsRememberZoom, mRememberZoom);
        if (mRememberZoom) {
            settings.setValue(kSettingsZoomMode, static_cast<int32_t>(mZoomMode));
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

void CanvasWidget::onActGammaType(bool checked, GammaType g)
{
    if (checked) {
        mGammaType = g;
        double value = 1.0;
        switch(g) {
        default:
        case GammaType::eLinear:
            break;
        case GammaType::eGamma22:
            value = 1.0 / 2.2;
            break;
        case GammaType::eDegamma22:
            value = 2.2;
            break;
        }
        if (mImageProcessor) {
            mImageProcessor->setGamma(value);
        }
        if (mImageDescription) {
            mImageDescription->setGammaValue(value);
            invalidateImageDescription();
        }
        update();
    }
}

void CanvasWidget::onActSwizzle(bool checked, ChannelSwizzle s)
{
    if (checked && mImageProcessor) {
        mImageProcessor->setChannelSwizzle(s);
        update();
    }
}

void CanvasWidget::onActTransparency(bool checked)
{
    if (mShowTransparencyCheckboard != checked) {
        mShowTransparencyCheckboard = checked;
        update();
    }
}
