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

#include "HistogramWidget.h"

#include <cmath>
#include <iostream>

#include <QPainter>
#include <QPainterPath>
#include <QGlyphRun>
#include <QMouseEvent>
#include <QLineSeries>
#include <QChart>
#include <QBoxLayout>
#include <QLegendMarker>
#include <QValueAxis>
#include <QGraphicsLayout>
#include <QGraphicsProxyWidget>

#include "Global.h"
#include "Histogram.h"
#include "ImagePage.h"
#include "DragCornerWidget.h"
#include "CanvasWidget.h"
#include "TextWidget.h"


namespace
{

    constexpr const int32_t kDragCornerSize = 16;


    std::vector<std::tuple<QString, QColor>> SelectChannelLabels(FIBITMAP* image, FIE_ImageFormat format)
    {
        if (image != nullptr) {
            if (format == FIE_ImageFormat::FIEF_FLO) {
                return { std::make_tuple("Motion X", QColorConstants::Red), std::make_tuple("Motion Y", QColorConstants::Blue) };
            }

            switch (FreeImage_GetImageType(image)) {
            case FIT_RGBAF:
            case FIT_RGBF:
            case FIT_RGBA16:
            case FIT_RGBA32:
            case FIT_RGB16:
            case FIT_RGB32:
                return { std::make_tuple("Red", QColorConstants::Red), std::make_tuple("Green", QColorConstants::Green), std::make_tuple("Blue", QColorConstants::Blue), std::make_tuple("Brightness", QColor(1, 1, 1)) };

            case FIT_UINT16:
            case FIT_INT16:
            case FIT_UINT32:
            case FIT_INT32:
            case FIT_FLOAT:
            case FIT_DOUBLE:
                return { std::make_tuple("Brightness", QColorConstants::Green) };

            case FIT_COMPLEXF:
            case FIT_COMPLEX:
                return { std::make_tuple("Real", QColorConstants::Red), std::make_tuple("Imag", QColorConstants::Blue), std::make_tuple("Abs", QColor(1, 1, 1)) };

            case FIT_BITMAP: {
                    const uint32_t bpp = FreeImage_GetBPP(image);
                    const auto colorType = FreeImage_GetColorType(image);
                    if ((32 == bpp) || (24 == bpp) || (FIC_PALETTE == colorType)) {
                        return { std::make_tuple("Red", QColorConstants::Red), std::make_tuple("Green", QColorConstants::Green), std::make_tuple("Blue", QColorConstants::Blue), std::make_tuple("Brightness", QColor(1, 1, 1)) };
                    }
                    else if (FIC_MINISWHITE == colorType || FIC_MINISBLACK == colorType) {
                        return { std::make_tuple("Brightness", QColorConstants::Green) };
                    }
                }
                break;

            default:
                break;
            }

        }
        return { std::make_tuple("Channel 1", QColorConstants::Red), std::make_tuple("Channel 2", QColorConstants::Green), std::make_tuple("Channel 3", QColorConstants::Blue), std::make_tuple("Brightness", QColor(1, 1, 1)) };
    }



} // namespace 

HistogramWidget::HistogramWidget(QWidget* parent)
    : QWidget(parent)
{
    //setAttribute(Qt::WA_TransparentForMouseEvents);
    setFocusPolicy(Qt::NoFocus);

    mHistogram = std::make_unique<Histogram>(256);

    mChartView = new QChartView(new QChart(), this);
    mChartView->setRenderHint(QPainter::Antialiasing);
    mChartView->setFrameShape(QFrame::NoFrame);
    mChartView->setAutoFillBackground(false);
    mChartView->setRubberBand(QChartView::NoRubberBand);
    mChartView->setStyleSheet("background: transparent");
    mChartView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mChartView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mChartView->setFocusPolicy(Qt::NoFocus);
    if (auto chart = mChartView->chart()) {
        chart->layout()->setContentsMargins(0, 0, 0, 0);
        chart->setBackgroundBrush(QColor(0x2B, 0x2B, 0x2B, 220));
    }

    auto layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mChartView);
    setLayout(layout);

    resize(640, 320);
    if (parentWidget()) {
        // top right corner
        moveSticky(QPoint(parentWidget()->width() - width(), 0));
    }

    mDragCorner = new DragCornerWidget(this, kDragCornerSize, QColor(0x4F, 0x4F, 0x4F, 220));
    connect(mDragCorner, &DragCornerWidget::draggingStart,  this, &HistogramWidget::onDraggedStart);
    connect(mDragCorner, &DragCornerWidget::draggingOffset, this, &HistogramWidget::onDraggedOffset);
    connect(mDragCorner, &DragCornerWidget::draggingStop,   this, &HistogramWidget::onDraggedStop);
    mDragCorner->resize(width(), kDragCornerSize);
    mDragCorner->setCornerRadius(6.0);
    auto dragLabel = new TextWidget(mDragCorner, QColorConstants::White, /*size = */ 10.0, /*pad = */0.5);
    dragLabel->setText("Histogram");
    dragLabel->setPaddings(2, 0, 0, 0);
    mDragCorner->show();

    mStretchCorner = new DragCornerWidget(this, kDragCornerSize, QColorConstants::Transparent);
    connect(mStretchCorner, &DragCornerWidget::draggingStart,  this, &HistogramWidget::onStretchedStart);
    connect(mStretchCorner, &DragCornerWidget::draggingOffset, this, &HistogramWidget::onStretchedOffset);
    connect(mStretchCorner, &DragCornerWidget::draggingStop,   this, &HistogramWidget::onStretchedStop);
    mStretchCorner->move(1, size().height() - kDragCornerSize - 1);
    auto stretchLabel = new TextWidget(mStretchCorner, QColorConstants::White, /*size = */ 14.0, /*pad = */0.5);
    stretchLabel->setText("\xE2\x87\xB2");
    stretchLabel->setMirroredHorz(true);
    mStretchCorner->show();

    if (auto canvas = dynamic_cast<CanvasWidget*>(parentWidget())) {
        connect(canvas, &CanvasWidget::eventResized, this, std::bind(&HistogramWidget::updatePositionOnResize, this));
    }
}

HistogramWidget::~HistogramWidget() = default;

void HistogramWidget::moveSticky(QPoint p, bool updateStickyFlags)
{
    if (updateStickyFlags) {
        mStickyLeft = (p.x() <= 0);
        mStickyTop = (p.y() <= 0);
        mStickyRight = (parentWidget() && p.x() >= parentWidget()->width() - width());
        mStickyBottom = (parentWidget() && p.y() >= parentWidget()->height() - height());
    }
    if (parentWidget()) {
        p.setX(std::min(p.x(), parentWidget()->width() - width()));
        p.setY(std::min(p.y(), parentWidget()->height() - height()));
    }
    p.setX(std::max(p.x(), 0));
    p.setY(std::max(p.y(), 0));
    move(p);
}

void HistogramWidget::attachImageSource(QWeakPointer<Image> image)
{
    if (auto oldImg = mImageSource.lock()) {
        oldImg->removeListener(this);
    }
    mImageSource = std::move(image);
    if (auto newImg = mImageSource.lock()) {
        newImg->addListener(this);
    }
    mIsValid = false;
    update();
}

void HistogramWidget::mousePressEvent(QMouseEvent* event)
{
    event->ignore();
}

void HistogramWidget::mouseMoveEvent(QMouseEvent* event)
{
    event->ignore();
}

void HistogramWidget::mouseReleaseEvent(QMouseEvent* event)
{
    event->ignore();
}

void HistogramWidget::updatePositionOnResize()
{
    QPoint p = pos();
    if (parentWidget()) {
        if (mStickyRight && !mStickyLeft) {
            p.setX(parentWidget()->width() - width());
        }
        if (mStickyBottom && !mStickyTop) {
            p.setY(parentWidget()->height() - height());
        }
    }
    moveSticky(p, /*updateStickyFlags=*/ false);
}

void HistogramWidget::onDraggedStart()
{
    mCurrentPos = pos();
}

void HistogramWidget::onDraggedOffset(QPoint offset)
{
    moveSticky(mCurrentPos + offset);
}

void HistogramWidget::onDraggedStop()
{

}

void HistogramWidget::onStretchedStart()
{
    mCurrentGeometry = geometry();
}

void HistogramWidget::onStretchedOffset(QPoint offset)
{
    const int32_t newWidth  = std::max(128, mCurrentGeometry.width()  - offset.x());
    const int32_t newHeight = std::max(128, mCurrentGeometry.height() + offset.y());

    QRect newGeometry{ };
    newGeometry.setX(mCurrentGeometry.right() - newWidth);
    newGeometry.setY(mCurrentGeometry.top());
    newGeometry.setWidth(newWidth);
    newGeometry.setHeight(newHeight);
    setGeometry(newGeometry);

    mDragCorner->resize(newWidth, kDragCornerSize);
    mStretchCorner->move(1, newHeight - kDragCornerSize - 1);
}

void HistogramWidget::onStretchedStop()
{

}

void HistogramWidget::keyPressEvent(QKeyEvent* event)
{
    event->ignore();
}

void HistogramWidget::keyReleaseEvent(QKeyEvent* event)
{
    event->ignore();
}

void HistogramWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    if (!mIsValid) {
        QPainter painter(this);
        painter.fillRect(this->rect(), QColorConstants::Transparent);


        FIBITMAP* srcImage{ nullptr };
        FIE_ImageFormat srcFormat = static_cast<FIE_ImageFormat>(FIF_UNKNOWN);

        if (auto image = mImageSource.lock()) {
            if (image->notNull()) {
                srcFormat = image->getSourceFormat();
                srcImage = (image->info().animated)
                    ? image->getBitmap()
                    : image->currentPage().getSourceBitmap();
            }
        }

        mHistogram->Reset();
        if (srcImage) {
            mHistogram->FillFromBitmap(srcImage);
        }

        if (auto chart = mChartView->chart()) {

            if (chart->axes().size() != 2) {
                while (!chart->axes().empty()) {
                    chart->removeAxis(chart->axes().back());
                }
                chart->addAxis(new QValueAxis(), Qt::AlignBottom);
                chart->addAxis(new QValueAxis(), Qt::AlignLeft);
            }

            auto xAxis = static_cast<QValueAxis*>(chart->axes(Qt::Horizontal).at(0));
            auto yAxis = static_cast<QValueAxis*>(chart->axes(Qt::Vertical).at(0));

            if (!mHistogram->Empty()) {
                const qreal yMultiplier = 100.0 / mHistogram->GetPixelsNumber();

                xAxis->setRange(mHistogram->minValue, mHistogram->maxValue);
                xAxis->setLabelsBrush(QColorConstants::White);
                xAxis->show();

                yAxis->setRange(0.0, yMultiplier * mHistogram->GetMaxBinValue());
                yAxis->setLabelsBrush(QColorConstants::White);
                yAxis->setLabelFormat("%.1f%%");
                yAxis->show();


                constexpr qreal kLineThickness = 2.0;
                const auto channelLegend = SelectChannelLabels(srcImage, srcFormat);
                const bool resetOpacity = (channelLegend.size() != chart->series().size());
                for (size_t chanIdx = 0; chanIdx < channelLegend.size(); ++chanIdx) {
                    QList<QPointF> points;
                    for (uint32_t i = 0; i < mHistogram->rgbl.size() / 4; ++i) {
                        points.append(QPointF(i, yMultiplier * mHistogram->rgbl[i * 4 + chanIdx]));
                    }

                    std::unique_ptr<QLineSeries> newSeries{ nullptr };
                    QLineSeries* s = nullptr;
                    if (chanIdx < chart->series().size()) {
                        s = static_cast<QLineSeries*>(chart->series().at(chanIdx));
                    }
                    else {
                        newSeries = std::make_unique<QLineSeries>();
                        s = newSeries.get();
                    }

                    s->replace(points);
                    s->setName(std::get<QString>(channelLegend.at(chanIdx)));
                    s->setPen(QPen(std::get<QColor>(channelLegend.at(chanIdx)), kLineThickness));
                    if (resetOpacity) {
                        s->setOpacity(1.0);
                    }

                    if (newSeries) {
                        chart->addSeries(newSeries.release());
                        s->attachAxis(yAxis);
                    }

                    s->show();
                }
                while (chart->series().size() > channelLegend.size()) {
                    chart->removeSeries(chart->series().back());
                }

                if (auto legend = chart->legend()) {
                    auto markers = legend->markers();
                    for (size_t i = 0; i < markers.size(); ++i) {
                        disconnect(markers.at(i), nullptr, this, nullptr);
                        connect(markers.at(i), &QLegendMarker::clicked, this, std::bind(&HistogramWidget::onMarkerClicked, this, markers.at(i)));
                        if (auto s = dynamic_cast<QLineSeries*>(markers.at(i)->series())) {
                            if (s->opacity() > 0.0) {
                                markers.at(i)->setLabelBrush(QColorConstants::White);
                                markers.at(i)->setBrush(s->color());
                            }
                        }
                        markers.at(i)->setVisible(true);
                    }
                    legend->show();
                }
            }
            else {
                // Histogram->isEmpty()

                xAxis->hide();
                yAxis->hide();

                for (auto& s : chart->series()) {
                    s->hide();
                }

                if (auto legend = chart->legend()) {
                    legend->hide();
                }
            }

            mChartView->update();
        }
        mIsValid = true;
    }
}

void HistogramWidget::onMarkerClicked(QLegendMarker* marker) const
{
    if (auto series = dynamic_cast<QLineSeries*>(marker->series())) {
        if (series->opacity() > 0.0) {
            QBrush b(QColor(127, 127, 127, 127));
            marker->setBrush(b);
            marker->setLabelBrush(b);
            series->setOpacity(0.0);
        }
        else {
            marker->setBrush(series->color());
            marker->setLabelBrush(QColorConstants::White);
            series->setOpacity(1.0);
        }
    }
}

void HistogramWidget::onInvalidated(Image*)
{
    mIsValid = false;
    update();
}
