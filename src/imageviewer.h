

#ifndef GUI_WIDGET_H
#define GUI_WIDGET_H

#include <chrono>

#include <QPixmap>
#include <QWidget>


class GuiWidget : public QWidget
{
    Q_OBJECT

public:
    GuiWidget(const std::string & path, std::chrono::steady_clock::time_point t);

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;

private:

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    int mClickX;
    int mClickY;

    std::chrono::steady_clock::time_point mStartTime;
    bool mStartup = true;
    QPixmap mPixmap;
};


#endif // GUI_WIDGET_H
