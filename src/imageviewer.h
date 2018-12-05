

#ifndef GUI_WIDGET_H
#define GUI_WIDGET_H

#include <chrono>
#include <memory>

#include <QPixmap>
#include <QWidget>
#include <QFuture>

class GuiWidget : public QWidget
{
    Q_OBJECT

public:
    GuiWidget(std::chrono::steady_clock::time_point t);

    void loadImageAsync(const QString & path);

private slots:
    void onImageReady(QPixmap p);
    void onFatalError();

signals:
    void eventImageReady(QPixmap p);
    void eventFatalError();

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


#endif // GUI_WIDGET_H
