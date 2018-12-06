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

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

    bool mVisible = false;
    std::unique_ptr<QPixmap> mPendingImage;

    int mClickX = 0;
    int mClickY = 0;

    std::chrono::steady_clock::time_point mStartTime;
    bool mStartup = true;
    QPixmap mPixmap;
};


#endif // CANVAS_WIDGET_H
