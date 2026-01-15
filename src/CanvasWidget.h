/**
 * @file
 *
 * Copyright 2018-2023 Alexey Gruzdev
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

#ifndef CANVAS_WIDGET_H
#define CANVAS_WIDGET_H

#include <array>
#include <chrono>
#include <memory>
#include <future>

#include <QFuture>
#include <QMenu>
#include <QPixmap>
#include <QTimer>
#include <QWidget>
#include <QWidgetAction>
#include <QSettings>
#include <QLabel>

#include "FreeImageExt.h"
#include "ImageProcessor.h"
#include "EnumArray.h"
#include "ImageDescription.h"

enum class BorderPosition;

class AboutWidget;
class Controls;
class HistogramWidget;
class ExifWidget;
class SettingsWidget;
class TextWidget;
class ToolbarButton;
class Tooltip;
class ZoomController;

struct ImageLoadResult;

enum class FilteringMode
{
    eNone,
    eAntialiasing,

    length_
};

enum class ZoomMode
{
    eIdentity,
    eFitWindow,
    eCustom,

    length_
};

enum class GammaType
{
    eLinear,
    eGamma22,
    eDegamma22,

    length_
};

class CanvasWidget final
    : public QWidget
{
    Q_OBJECT

    template <typename Enum_, size_t Length_ = static_cast<size_t>(Enum_::length_)>
    using ActionsArray = EnumArray<QWidgetAction*, Enum_, Length_>;

    using TMActionsArray = ActionsArray<FREE_IMAGE_TMO, 5>;

public:
    CanvasWidget(std::chrono::steady_clock::time_point t);
    ~CanvasWidget();

    QRect getAvailableSpace() const;

public slots:
    void onImageReady(const ImageLoadResult& result);
    void onImageDirScanned(size_t imgIdx, size_t totalCount);

    void onTransitionCanceled();

    void applicationStateChanged(Qt::ApplicationState state);

    void onHoverInterruted();

    // Actions
    void onActNoFilter(bool checked);
    void onActAntialiasing(bool checked);

    void onActRotation(bool checked, Rotation r);
    void onActFlip(bool checked, FlipType f);
    void onActZoomMode(bool checked, ZoomMode z);
    void onActRememberZoom(bool checked);
    void onActToneMapping(bool checked, FREE_IMAGE_TMO m);
    void onActGammaType(bool checked, GammaType g);
    void onActSwizzle(bool checked, ChannelSwizzle s);
    void onActTransparency(bool checked);

    void onShowContextMenu(const QPoint &pos);

    void onAnimationTick(uint64_t imgId);

    void onSettingsChanged();

signals:
    void eventInfoText(QString s);

    void eventNextImage();
    void eventPrevImage();
    void eventFirstImage();
    void eventLastImage();
    void eventReloadImage();
    void eventOpenImage();

    void eventResized();

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
    void keyReleaseEvent(QKeyEvent* event) Q_DECL_OVERRIDE;

private:
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void wheelEvent(QWheelEvent* event) Q_DECL_OVERRIDE;
    void leaveEvent(QEvent* event) Q_DECL_OVERRIDE;
    void closeEvent(QCloseEvent* event) Q_DECL_OVERRIDE;

    void invalidateImageDescription();
    void updateZoomLabel();

    QRect calculateImageRegion() const;
    void setGeometry2(QRect r);

    void resetOffsets();
    void updateOffsets();

    QRect fitWidth(int w, int h) const;

    void recalculateFittingScale();

    bool setFullscreenGeometry();

    void zoomToTarget(QPoint target, int dir);

    void repositionPageText();

    QWidgetAction* createMenuAction(const QString & text);

    ActionsArray<Rotation> initRotationActions();
    ActionsArray<FlipType> initFlipActions();
    ActionsArray<ZoomMode> initZoomActions();
    TMActionsArray initToneMappingActions();
    ActionsArray<GammaType> initGammaTypeActions();
    ActionsArray<ChannelSwizzle> initSwizzleActions();

    QMenu* createContextMenu();

    void invalidateTooltip();

    void invalidateExif();

private:
    std::unique_ptr<QSettings> mSettings;
    bool mLocalSettingsAreInvalidated = true;

    ImagePtr mImage;

    std::unique_ptr<ImageDescription> mImageDescription;
    bool mDisplayFullPath = false;

    std::unique_ptr<ImageProcessor> mImageProcessor;

    bool mTransitionRequested = true;
    bool mTransitionIsReload = false;

    bool mFullScreen = false;

    bool mClick = false;

    bool mDragging = false;
    QPoint mMenuPos { 0, 0 };
    QPoint mClickPos{ 0, 0 };

    bool mStretching = false;
    BorderPosition mHoveredBorder;
    QRect mClickGeometry;

    std::chrono::steady_clock::time_point mStartTime;
    bool mStartup = true;

    bool mShowInfo = false;

    QPoint mOffset{ 0, 0 };

    std::unique_ptr<ZoomController> mZoomController;
    ZoomMode mZoomMode;
    bool mRememberZoom = false;

    bool mBrowsing = false;

    QPoint mCursorPosition;

    TextWidget* mInfoText  = nullptr;
    bool mInfoIsValid = false;

    TextWidget* mErrorText = nullptr;

    TextWidget* mPageText = nullptr;

    std::unique_ptr<Tooltip> mTooltip;

    FilteringMode mFilteringMode;

    GammaType mGammaType = GammaType::eLinear;

    QMenu* mContextMenu = nullptr;

    bool mEnableAnimation = true;
    uint32_t mAnimIndex = 0;

    bool mShowTransparencyCheckboard = false;
    std::shared_future<QPixmap> mCheckboard;

    HistogramWidget* mHistogramWidget = nullptr;

    // Actions
    std::shared_future<ActionsArray<Rotation>> mActRotate;
    std::shared_future<ActionsArray<FlipType>> mActFlip;
    std::shared_future<ActionsArray<ZoomMode>> mActZoom;
    std::shared_future<TMActionsArray> mActToneMapping;
    std::shared_future<ActionsArray<GammaType>> mActGammaType;
    std::shared_future<ActionsArray<ChannelSwizzle>> mActSwizzle;


    // Toolbar buttons
    QWidget* mButtonsArea = nullptr;
    ToolbarButton* mCloseButton = nullptr;

    // Extra windows
    std::unique_ptr<AboutWidget> mAboutWidget = nullptr;
    std::unique_ptr<ExifWidget> mExifWidget = nullptr;
    std::unique_ptr<SettingsWidget> mSettingsWidget = nullptr;
};


#endif // CANVAS_WIDGET_H
