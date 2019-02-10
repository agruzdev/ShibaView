/**
* ShibaView
*
* The MIT License (MIT)
* Copyright (c) 2018 Alexey Gruzdev
*/
#ifndef MENUWIDGET_H
#define MENUWIDGET_H

#include <QWidget>
#include <QLayout>

class TextWidget;

class MenuWidget
    : public QWidget
{
    Q_OBJECT

signals:
    void toggled(bool checked) const;
    void activated() const;

public slots:
    void onActionToggled(bool checked);

public:
    MenuWidget(QWidget *parent = nullptr);
    MenuWidget(const QString &text, QWidget *parent = nullptr);

    ~MenuWidget();

    QSize minimumSizeHint() const Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent* e) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent* evt) Q_DECL_OVERRIDE;

private:
    void init();

    QString text = "";
    bool checked = false;

    QLayout* mLayout;
    TextWidget* mBulletWidget;
    TextWidget* mTextWidget;
};




#endif // MENUWIDGET_H
