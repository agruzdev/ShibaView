/**
* ShibaView
*
* The MIT License (MIT)
* Copyright (c) 2018 Alexey Gruzdev
*/

#ifndef TEXTWIDGET_H
#define TEXTWIDGET_H

#include <QWidget>
#include <QFont>
#include <QRawFont>
#include <QPen>
#include <QBrush>
#include <QGraphicsDropShadowEffect>

class TextWidget 
    : public QWidget
{
    Q_OBJECT
public:
    explicit TextWidget(QWidget* parent = nullptr);
    TextWidget(QWidget* parent, Qt::GlobalColor color, int fsize = 14);
    ~TextWidget();

    uint32_t textWidth() const
    {
        return mWidth;
    }

    uint32_t textHeight() const
    {
        return mLineHeight;
    }

    void enableShadow();

public slots:
    void setText(const QString & line);
    void setText(const QVector<QString> & lines);
    void setLine(uint32_t idx, const QString & line);

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

private:
    void autoResize();

    QRawFont mRawFont;
    QPen mPen;
    QBrush mBrush;

    QVector<QString> mLines;

    uint32_t mLineHeight;
    uint32_t mWidth;

    QPoint mPadding;

    QGraphicsDropShadowEffect* mShadow = nullptr;
};

#endif // TEXTWIDGET_H