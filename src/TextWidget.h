/**
 * @file
 *
 * Copyright 2018-2023 Alexey Gruzdev
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef TEXTWIDGET_H
#define TEXTWIDGET_H

#include <limits>
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
    TextWidget(QWidget* parent, Qt::GlobalColor color, qreal fsize = 14.0, qreal padh = 1.0);
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

    void setBackgroundColor(QColor color)
    {
        mBackgroundColor = std::move(color);
    }

    void setBorderColor(QColor color)
    {
        mBorderColor = std::move(color);
    }

    void setPaddings(int32_t left, int32_t right, int32_t top, int32_t bottom)
    {
        mPaddings.setLeft(left);
        mPaddings.setRight(right);
        mPaddings.setTop(top);
        mPaddings.setBottom(bottom);
    }

    void setColor(QColor c)
    {
        mPen   = QPen(c);
        mBrush = QBrush(c, Qt::BrushStyle::SolidPattern);
    }

    bool setColumnSeperator(QChar c);

    void appendColumnOffset(qreal offset)
    {
        mColumnOffsets.push_back(offset);
    }

public slots:
    void setText(const QString & line);
    void setText(const QVector<QString> & lines);
    void setLine(uint32_t idx, const QString & line);

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

private:
    void autoResize();

    QRawFont mRawFont;

    QPen   mPen;
    QPen   mPenDisabled;
    QBrush mBrush;
    QBrush mBrushDisabled;

    QVector<QString> mLines;

    qreal mGlyphPadH = 3.75;
    qreal mGlyphPadV = 5.0;

    qreal mLineHeight;
    qreal mWidth;

    QRect mPaddings = QRect(0, 0, 0, 0);

    QGraphicsDropShadowEffect* mShadow = nullptr;

    QColor mBorderColor = Qt::transparent;
    QColor mBackgroundColor = Qt::transparent;

    uint mColumnSeparator = std::numeric_limits<uint>::max();
    QVector<qreal> mColumnOffsets;
};

#endif // TEXTWIDGET_H