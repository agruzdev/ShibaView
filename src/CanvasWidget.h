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
#include <QPixmap>
#include <QWidget>
#include <QFuture>
#include <QSizeGrip>
#include <ImageLoader.h>

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
    void onImageReady(QPixmap p, const ImageInfo & i);

    void onTransitionCanceled();

    // Actions
    void onActNoFilter(bool checked);
    void onActAntialiasing(bool checked);

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

    void updateOffsets();
    QRect fitWidth(int w, int h) const;

    void recalculateZoom();

    void setFullscreenGeometry();

    void zoomToTarget(QPoint target, int dir);

    bool mVisible = false;
    std::unique_ptr<QPixmap> mPendingImage;

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

    QPixmap mPixmap;
    ImageInfo mImageInfo;

    QRect mImageRegion;

    bool mShowInfo = false;

    std::unique_ptr<ZoomController> mZoomController;
    ZoomMode mZoomMode;

    bool mBrowsing = false;

    QPoint mCursorPosition;

    TextWidget* mInfoText  = nullptr;
    TextWidget* mErrorText = nullptr;

    FilteringMode mFilteringMode;

    // Menu actions
    std::unique_ptr<QActionGroup> mActGroupFiltering;
    std::unique_ptr<QAction> mActNoFilter;
    std::unique_ptr<QAction> mActAntialiasing;
};


#endif // CANVAS_WIDGET_H
