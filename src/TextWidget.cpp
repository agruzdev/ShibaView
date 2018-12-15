#include "TextWidget.h"

#include <QPainter>

TextWidget::TextWidget(QWidget *parent) 
    : QWidget(parent)
{
    mRawFont = QRawFont(QString(":/fonts/ArchivoNarrow-Regular.otf"), 16);
    if(!mRawFont.isValid()) {
        mRawFont = QRawFont::fromFont(QFont());
    }
    mPen     = QPen(Qt::white);
    mBrush   = QBrush(Qt::white, Qt::BrushStyle::SolidPattern);

    mLineHeight = mRawFont.capHeight() + 12;
}

TextWidget::~TextWidget()
{
    
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
    uint32_t width = 1;
    for (int32_t i = 0; i < mLines.size(); ++i) {
        qreal lineWidth = 0;
        auto glyphs = mRawFont.glyphIndexesForString(mLines[i]);
        for (int32_t j = 0; j < glyphs.size(); ++j) {
            const auto path = mRawFont.pathForGlyph(glyphs[j]);
            lineWidth += path.boundingRect().width() + 4;
        }
        width = std::max<uint32_t>(width, std::ceil(lineWidth));
    }
    resize(width, mLines.size() * mLineHeight);
}

void TextWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.setPen(mPen);
    painter.setBrush(mBrush);
    painter.setRenderHint(QPainter::RenderHint::Antialiasing);

    for (int32_t i = 0; i < mLines.size(); ++i) {
        if(mLines[i].size() > 0) {
            auto glyphs = mRawFont.glyphIndexesForString(mLines[i]);
            painter.resetTransform();
            painter.scale(1.0, 0.9);
            painter.translate(0, (i + 1) * mLineHeight);
            for (int32_t j = 0; j < glyphs.size(); ++j) {
                const auto path = mRawFont.pathForGlyph(glyphs[j]);
                painter.drawPath(path);
                painter.translate(path.boundingRect().width() + 4, 0);
            }
        }
    }
}
