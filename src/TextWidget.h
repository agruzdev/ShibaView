#ifndef TEXTWIDGET_H
#define TEXTWIDGET_H

#include <QWidget>
#include <QFont>
#include <QRawFont>
#include <QPen>
#include <QBrush>

class TextWidget 
    : public QWidget
{
    Q_OBJECT
public:
    explicit TextWidget(QWidget *parent = nullptr);
    ~TextWidget();

signals:

public slots:
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

    QPoint mPadding;
};

#endif // TEXTWIDGET_H