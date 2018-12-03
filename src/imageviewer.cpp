#include "imageviewer.h"

#include <QPainter>
#include <QKeyEvent>
#include <QDateTime>

#include <iostream>

#include "FreeImage.h"

GuiWidget::GuiWidget(const std::string & path, std::chrono::steady_clock::time_point t)
    : QWidget(nullptr)
    , mStartTime(std::move(t))
{
    auto fif = FreeImage_GetFIFFromFilename(path.c_str());
    auto img = FreeImage_Load(fif, path.c_str());
    if(img == nullptr){
        throw std::runtime_error("Failed to open image");
    }
    FreeImage_FlipVertical(img);

    const uint32_t width  = FreeImage_GetWidth(img);
    const uint32_t height = FreeImage_GetHeight(img);

    resize(width, height);
    window()->setFixedSize(width, height);

    switch(FreeImage_GetImageType(img)){
    case FIT_BITMAP:
        if(32 == FreeImage_GetBPP(img)) {
            QImage qimg(FreeImage_GetBits(img), width, height, FreeImage_GetPitch(img), QImage::Format_RGBA8888);
            mPixmap = QPixmap::fromImage(qimg);
            break;
        }
        else if(24 == FreeImage_GetBPP(img)) {
            QImage qimg(FreeImage_GetBits(img), width, height, FreeImage_GetPitch(img), QImage::Format_RGB888);
            mPixmap = QPixmap::fromImage(qimg);
            break;
        }
    default:
        throw std::runtime_error("Unsupported image format");
    }
}

void GuiWidget::paintEvent(QPaintEvent * /* event */)
{
    if(mStartup){
        std::cout << (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - mStartTime).count() / 1e3) << std::endl;
        mStartup = false;
    }

    QPainter painter(this);
    painter.drawPixmap(QPoint(), mPixmap);
}

void GuiWidget::resizeEvent(QResizeEvent * /* event */)
{

}

void GuiWidget::keyPressEvent(QKeyEvent* event)
{
    if(event->key() == Qt::Key_Escape) {
        close();
    }
}

void GuiWidget::mousePressEvent(QMouseEvent* event) {
    mClickX = event->x();
    mClickY = event->y();
}

void GuiWidget::mouseMoveEvent(QMouseEvent* event) {
    move(event->globalX() - mClickX, event->globalY() - mClickY);
}



