/**
* ShibaView
*
* The MIT License (MIT)
* Copyright (c) 2018 Alexey Gruzdev
*/

#ifndef CANVAS_WIDGET_H
#define CANVAS_WIDGET_H

#include <chrono>
#include <memory>

#include <QAction>
#include <QActionGroup>
#include <QMenu>
#include <QPixmap>
#include <QWidget>
#include <QFuture>

#include <Image.h>

enum class BorderPosition;

class QLabel;
class ZoomController;
class TextWidget;

enum class ZoomMode
{
    eFitWidth,
    e100Percent,
    eCustom
};

enum class FilteringMode
{
    eNone,
    eAntialiasing
};


class CanvasWidget
    : public QWidget
{
    Q_OBJECT

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

    void onShowContextMenu(const QPoint &pos);

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

    // Reset image region and zoom controller for new image extents
    void invalidateImageExtents(bool keepTransform = true);

    void updateOffsets();
    QRect fitWidth(int w, int h) const;

    void recalculateZoom();

    void setFullscreenGeometry();

    void zoomToTarget(QPoint target, int dir);

    QSharedPointer<Image> mPendingImage;
    QSharedPointer<Image> mImage;

    bool mVisible = false;

    bool mTransitionRequested = true;

    bool mFullScreen = false;

    bool mClick = false;

    bool mDragging = false;
    int mClickX = 0;
    int mClickY = 0;

    bool mStretching = false;
    BorderPosition mHoveredBorder;
    QRect mClickGeometry;

    std::chrono::steady_clock::time_point mStartTime;
    bool mStartup = true;

    QRect mImageRegion;

    bool mShowInfo = false;

    std::unique_ptr<ZoomController> mZoomController;
    ZoomMode mZoomMode;

    bool mBrowsing = false;

    QPoint mCursorPosition;

    TextWidget* mInfoText  = nullptr;
    TextWidget* mErrorText = nullptr;

    FilteringMode mFilteringMode;

    QMenu mContextMenu;
};


#endif // CANVAS_WIDGET_H
