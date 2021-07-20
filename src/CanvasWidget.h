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

#include "FreeImageExt.h"
#include "ImageProcessor.h"
#include "EnumArray.h"
#include "ImageDescription.h"

enum class BorderPosition;

class QLabel;

class Controls;
class TextWidget;
class Tooltip;
class ZoomController;

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
    eFixed,

    length_
};

class CanvasWidget final
    : public QWidget
{
    Q_OBJECT

    template <typename Enum_, size_t Length_ = static_cast<size_t>(Enum_::length_)>
    using ActionsArray = EnumArray<QWidgetAction*, Enum_, Length_>;

    using TMActionsArray = ActionsArray<FIE_ToneMapping, 5>;

public:
    CanvasWidget(std::chrono::steady_clock::time_point t);
    ~CanvasWidget();

public slots:
    void onImageReady(ImagePtr image, size_t imgIdx, size_t totalCount);
    void onImageDirScanned(size_t imgIdx, size_t totalCount);

    void onTransitionCanceled();

    void applicationStateChanged(Qt::ApplicationState state);

    // Actions
    void onActNoFilter(bool checked);
    void onActAntialiasing(bool checked);

    void onActRotation(bool checked, Rotation r);
    void onActFlip(bool checked, FlipType f);
    void onActZoomMode(bool checked, ZoomMode z);
    void onActToneMapping(bool checked, FIE_ToneMapping m);
    void onActSwizzle(bool checked, ChannelSwizzle s);

    void onShowContextMenu(const QPoint &pos);

    void onAnimationTick(uint64_t imgId);

signals:
    void eventInfoText(QString s);

    void eventNextImage();
    void eventPrevImage();
    void eventFirstImage();
    void eventLastImage();
    void eventReloadImage();
    void eventOpenImage();

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

    void invalidateImageDescription();
    void updateZoomLabel();

    QRect calculateImageRegion() const;

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
    ActionsArray<ChannelSwizzle> initSwizzleActions();

    QMenu* createContextMenu();

    void invalidateTooltip();

    void invalidateExif();

private:
    QColor mBackgroundColor;

    ImagePtr mImage;

    std::unique_ptr<ImageDescription> mImageDescription;

    std::unique_ptr<ImageProcessor> mImageProcessor;

    bool mTransitionRequested = true;

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

    bool mBrowsing = false;

    QPoint mCursorPosition;

    TextWidget* mInfoText  = nullptr;
    bool mInfoIsValid = false;

    TextWidget* mErrorText = nullptr;

    TextWidget* mPageText = nullptr;

    std::unique_ptr<Tooltip> mTooltip;

    FilteringMode mFilteringMode;

    QMenu* mContextMenu = nullptr;

    bool mEnableAnimation = true;
    uint32_t mAnimIndex = 0;

    // Actions
    std::shared_future<ActionsArray<Rotation>> mActRotate;
    std::shared_future<ActionsArray<FlipType>> mActFlip;
    std::shared_future<ActionsArray<ZoomMode>> mActZoom;
    std::shared_future<TMActionsArray> mActToneMapping;
    std::shared_future<ActionsArray<ChannelSwizzle>> mActSwizzle;
};


#endif // CANVAS_WIDGET_H
