/**
 * @file
 *
 * Copyright 2018-2020 Alexey Gruzdev
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

#include "TextWidget.h"

#include <cmath>

#include <QPainter>
#include <QPainterPath>
#include <QGlyphRun>

#include "Global.h"

namespace
{
    Q_DECL_CONSTEXPR uint32_t FONT_HPADDING = 4;
    Q_DECL_CONSTEXPR uint32_t FONT_VPADDING = 5;
}

TextWidget::TextWidget(QWidget *parent)
    : TextWidget(parent, Qt::white)
{ }

TextWidget::TextWidget(QWidget* parent, Qt::GlobalColor color, int fsize)
    : QWidget(parent)
{
    mRawFont = QRawFont(Global::defaultFont, fsize);
    if(!mRawFont.isValid()) {
        mRawFont = QRawFont::fromFont(QFont());
    }
    mPen           = QPen(color);
    mPenDisabled   = QPen(Qt::gray);
    mBrush         = QBrush(color,    Qt::BrushStyle::SolidPattern);
    mBrushDisabled = QBrush(Qt::gray, Qt::BrushStyle::SolidPattern);

    mLineHeight = mRawFont.capHeight() + 2 * FONT_VPADDING;

    setAttribute(Qt::WA_TransparentForMouseEvents);
}

TextWidget::~TextWidget() = default;

void TextWidget::setText(const QString & line)
{
    mLines = QVector<QString>{ line };
    autoResize();
}

void TextWidget::setText(const QVector<QString> & lines)
{
    mLines = lines;
    autoResize();
}

void TextWidget::setLine(uint32_t idx, const QString & line)
{
    if (static_cast<int>(idx) < mLines.size()) {
        mLines[idx] = line;
        autoResize();
    }
}

void TextWidget::autoResize()
{
    mWidth = 1;
    for (const auto & line : mLines) {
        qreal lineWidth = 0;
        for (const auto & glyph : mRawFont.glyphIndexesForString(line)) {
            const auto path = mRawFont.pathForGlyph(glyph);
            lineWidth += path.boundingRect().width() + FONT_HPADDING;
        }
        mWidth = std::max<uint32_t>(mWidth, std::ceil(lineWidth));
    }
    const int32_t w = mWidth + mPaddings.left() + mPaddings.right();
    const int32_t h = mLines.size() * mLineHeight + FONT_VPADDING + mPaddings.top() + mPaddings.bottom();
    resize(w, h);
}

void TextWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter painter(this);

    if (mBackgroundColor != Qt::transparent) {
        painter.fillRect(this->rect(), mBackgroundColor);
    }

    if (mBorderColor != Qt::transparent) {
        painter.setPen(QPen(mBorderColor));
        QRect r = this->rect();
        r.setWidth(r.width() - 1);
        r.setHeight(r.height() - 1);
        painter.drawRect(r);
    }

    if(isEnabled()) {
        painter.setPen(mPen);
        painter.setBrush(mBrush);
    }
    else {
        painter.setPen(mPenDisabled);
        painter.setBrush(mBrushDisabled);
    }

    painter.setRenderHint(QPainter::RenderHint::Antialiasing);

    QGlyphRun glyphRun;
    glyphRun.setRawFont(mRawFont);
    for (int32_t i = 0; i < mLines.size(); ++i) {
        if (mLines[i].size() > 0) {
            auto glyphs = mRawFont.glyphIndexesForString(mLines[i]);
            painter.resetTransform();
            painter.translate(mPaddings.left(), mPaddings.top() + (i + 1) * mLineHeight - FONT_VPADDING);
            for (const auto & glyph : glyphs) {
                const auto path = mRawFont.pathForGlyph(glyph);
                painter.drawPath(path);
                painter.translate(path.boundingRect().width() + FONT_HPADDING, 0);
            }
        }
    }
}

void TextWidget::enableShadow()
{
    QGraphicsDropShadowEffect *eff = new QGraphicsDropShadowEffect(this);
    eff->setOffset(-1, 0);
    eff->setBlurRadius(5.0);
    eff->setColor(Qt::black);
    this->setGraphicsEffect(eff);
}
