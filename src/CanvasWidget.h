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

#include <Image.h>
#include <EnumArray.h>

enum class BorderPosition;

class QLabel;
class ZoomController;
class TextWidget;

enum class FilteringMode
{
    eNone,
    eAntialiasing,

    length_
};

enum class ZoomMode
{
    eFree,
    eFitWindow,
    eFixed,

    length_
};

class CanvasWidget
    : public QWidget
{
    Q_OBJECT

    template <typename Enum_>
    using ActionsArray = EnumArray<QWidgetAction*, Enum_>;

public:
    CanvasWidget(std::chrono::steady_clock::time_point t);
    ~CanvasWidget();

public slots:
    void onImageReady(ImagePtr image);

    void onTransitionCanceled();

    // Actions
    void onActNoFilter(bool checked);
    void onActAntialiasing(bool checked);

    void onActRotation(bool checked, Rotation r);
    void onActZoomMode(bool checked, ZoomMode z);

    void onShowContextMenu(const QPoint &pos);

    void onAnimationTick(uint64_t imgId);

signals:
    void eventInfoText(QString s);

    void eventNextImage();
    void eventPrevImage();
    void eventFirstImage();
    void eventLastImage();


protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;


private:
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void wheelEvent(QWheelEvent* event) Q_DECL_OVERRIDE;

    void updateZoomLabel();

    QRect calculateImageRegion() const;

    void resetOffsets();
    void updateOffsets();

    QRect fitWidth(int w, int h) const;

    void recalculateFittingScale();

    void setFullscreenGeometry();

    void zoomToTarget(QPoint target, int dir);

    void repositionPageText();

    QWidgetAction* createMenuAction(const QString & text);

    ActionsArray<Rotation> initRotationActions();
    ActionsArray<ZoomMode> initZoomActions();

    QMenu* createContextMenu();

    QSharedPointer<Image> mPendingImage;
    QSharedPointer<Image> mImage;

    bool mVisible = false;

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
    TextWidget* mErrorText = nullptr;

    TextWidget* mPageText = nullptr;

    FilteringMode mFilteringMode;

    QMenu* mContextMenu = nullptr;

    uint32_t mCurrPage = Image::kNonePage;

    // actions
    std::shared_future<ActionsArray<Rotation>> mActRotate;
    std::shared_future<ActionsArray<ZoomMode>> mActZoom;
};


#endif // CANVAS_WIDGET_H
