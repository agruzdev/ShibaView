/**
* ShibaView
*
* The MIT License (MIT)
* Copyright (c) 2018 Alexey Gruzdev
*/

#include "TextWidget.h"

#include <cmath>

#include <QPainter>
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
    mPen     = QPen(color);
    mBrush   = QBrush(color, Qt::BrushStyle::SolidPattern);

    mLineHeight = mRawFont.capHeight() + 2 * FONT_VPADDING;
}

TextWidget::~TextWidget()
{
    
}

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
    if(static_cast<int>(idx) < mLines.size()) {
        mLines[idx] = line;
    }
    autoResize();
}

void TextWidget::autoResize()
{
    mWidth = 1;
    for (int32_t i = 0; i < mLines.size(); ++i) {
        qreal lineWidth = 0;
        auto glyphs = mRawFont.glyphIndexesForString(mLines[i]);
        for (int32_t j = 0; j < glyphs.size(); ++j) {
            const auto path = mRawFont.pathForGlyph(glyphs[j]);
            lineWidth += path.boundingRect().width() + FONT_HPADDING;
        }
        mWidth = std::max<uint32_t>(mWidth, std::ceil(lineWidth));
    }
    resize(mWidth, mLines.size() * mLineHeight + FONT_VPADDING);
}

void TextWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.setPen(mPen);
    painter.setBrush(mBrush);
    painter.setRenderHint(QPainter::RenderHint::Antialiasing);

    QGlyphRun glyphRun;
    glyphRun.setRawFont(mRawFont);
    for (int32_t i = 0; i < mLines.size(); ++i) {
        if (mLines[i].size() > 0) {
            auto glyphs = mRawFont.glyphIndexesForString(mLines[i]);
            painter.resetTransform();
            painter.translate(0, (i + 1) * mLineHeight - FONT_VPADDING);
            for (int32_t j = 0; j < glyphs.size(); ++j) {
                const auto path = mRawFont.pathForGlyph(glyphs[j]);
                painter.drawPath(path);
                painter.translate(path.boundingRect().width() + FONT_HPADDING, 0);
            }
        }
    }
}
