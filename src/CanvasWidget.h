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

#include <QPixmap>
#include <QWidget>
#include <QFuture>
#include <QSizeGrip>

enum class BorderPosition;

class ZoomController;

class CanvasWidget
    : public QWidget
{
    Q_OBJECT

public:
    CanvasWidget(std::chrono::steady_clock::time_point t);
    ~CanvasWidget();

public slots:
    void onImageReady(QPixmap p);

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

    void updateSettings();
    void updateOffsets();

    bool mVisible = false;
    std::unique_ptr<QPixmap> mPendingImage;

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

    QRect mImageRegion;

    std::unique_ptr<ZoomController> mZoomController;

    bool mBrowsing = false;

    QPoint mCursorPosition;
};


#endif // CANVAS_WIDGET_H
