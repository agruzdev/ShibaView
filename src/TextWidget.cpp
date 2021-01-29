/**
 * @file
 *
 * Copyright 2018-2021 Alexey Gruzdev
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

TextWidget::TextWidget(QWidget *parent)
    : TextWidget(parent, Qt::white)
{ }

TextWidget::TextWidget(QWidget* parent, Qt::GlobalColor color, qreal fsize, qreal padh)
    : QWidget(parent)
{
    mRawFont = QRawFont(Global::kDefaultFont, fsize);
    if(!mRawFont.isValid()) {
        mRawFont = QRawFont::fromFont(QFont());
    }
    mPen           = QPen(color, 0.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    mPenDisabled   = QPen(Qt::gray);
    mBrush         = QBrush(color,    Qt::BrushStyle::SolidPattern);
    mBrushDisabled = QBrush(Qt::gray, Qt::BrushStyle::SolidPattern);

    mGlyphPadH *= padh;
    mLineHeight = mRawFont.capHeight() + 2.0 * mGlyphPadV;

    setAttribute(Qt::WA_TransparentForMouseEvents);
}

TextWidget::~TextWidget() = default;

void TextWidget::setText(const QString& line)
{
    mLines = QVector<QString>{ line };
    autoResize();
}

void TextWidget::setText(const QVector<QString>& lines)
{
    mLines = lines;
    autoResize();
}

void TextWidget::setLine(uint32_t idx, const QString& line)
{
    if (static_cast<int>(idx) < mLines.size()) {
        mLines[idx] = line;
        autoResize();
    }
}

bool TextWidget::setColumnSeperator(QChar c)
{
    quint32 glyph = 0;
    int glyphsCount = 1;
    if (mRawFont.glyphIndexesForChars(&c, 1, &glyph, &glyphsCount) && glyphsCount == 1) {
        mColumnSeparator = glyph;
        return true;
    }
    return false;
}

void TextWidget::autoResize()
{
    mWidth = 1.0;
    for (const auto & line : mLines) {
        qreal lineWidth = 0.0;
        if (!mColumnOffsets.empty()) {
            uint32_t columnIndex = 0;
            for (const auto& glyph : mRawFont.glyphIndexesForString(line)) {
                if (glyph == mColumnSeparator && columnIndex < mColumnOffsets.size()) {
                    lineWidth = mColumnOffsets[columnIndex++];
                }
                else {
                    const auto path = mRawFont.pathForGlyph(glyph);
                    lineWidth += path.boundingRect().width() + mGlyphPadH;
                }
            }
        }
        else {
            for (const auto& glyph : mRawFont.glyphIndexesForString(line)) {
                const auto path = mRawFont.pathForGlyph(glyph);
                lineWidth += path.boundingRect().width() + mGlyphPadH;
            }
        }
        mWidth = std::max(mWidth, lineWidth);
    }
    const qreal w = mWidth + mPaddings.left() + mPaddings.right();
    const qreal h = mLines.size() * mLineHeight + mGlyphPadV + mPaddings.top() + mPaddings.bottom();
    resize(static_cast<int>(std::ceil(w)), static_cast<int>(std::ceil(h)));
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
            const qreal lineOffsetY = mPaddings.top() + static_cast<qreal>(i + 1) * mLineHeight - mGlyphPadV;
            auto glyphs = mRawFont.glyphIndexesForString(mLines[i]);
            painter.resetTransform();
            painter.translate(mPaddings.left(), lineOffsetY);
            if (!mColumnOffsets.empty()) {
                uint32_t columnIndex = 0;
                for (const auto & glyph : glyphs) {
                    if (glyph == mColumnSeparator && columnIndex < mColumnOffsets.size()) {
                        painter.resetTransform();
                        painter.translate(mColumnOffsets[columnIndex++], lineOffsetY);
                    }
                    else {
                        const auto path = mRawFont.pathForGlyph(glyph);
                        painter.drawPath(path);
                        painter.translate(path.boundingRect().width() + mGlyphPadH, 0.0);
                    }
                }
            }
            else {
                for (const auto & glyph : glyphs) {
                    const auto path = mRawFont.pathForGlyph(glyph);
                    painter.drawPath(path);
                    painter.translate(path.boundingRect().width() + mGlyphPadH, 0.0);
                }
            }
        }
    }
}

void TextWidget::enableShadow()
{
    QGraphicsDropShadowEffect *eff = new QGraphicsDropShadowEffect(this);
    eff->setOffset(-1.0, 0.0);
    eff->setBlurRadius(5.0);
    eff->setColor(Qt::black);
    this->setGraphicsEffect(eff);
}
