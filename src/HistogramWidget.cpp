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
    //auto dragLabel = new TextWidget(mDragCorner, QColorConstants::White, /*size = */ 12.0, /*pad = */0.5);
    //dragLabel->setText("X");
    //dragLabel->move(mDragCorner->width() - dragLabel->width(), 0);
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
    QPainter painter(this);
    painter.fillRect(this->rect(), QColorConstants::Transparent);

    if (auto image = mImageSource.lock()) {
        if (!mIsValid) {
            if (image->info().animated) {
                mHistogram->FillFromBitmap(image->getBitmap());
            }
            else {
                mHistogram->FillFromBitmap(image->currentPage().getSourceBitmap());
            }

            if (auto chart = mChartView->chart()) {
                std::array<QLineSeries*, 4> series = { new QLineSeries(), new QLineSeries(), new QLineSeries(), new QLineSeries() };
                for (uint32_t i = 0; i < mHistogram->rgbl.size() / 4; ++i) {
                    for (uint32_t c = 0; c < 4; ++c) {
                        series[c]->append(i, mHistogram->rgbl[i * 4 + c]);
                    }
                }
                series[0]->setName("red");
                series[0]->setColor(QColorConstants::Red);
                series[1]->setName("green");
                series[1]->setColor(QColorConstants::Green);
                series[2]->setName("blue");
                series[2]->setColor(QColorConstants::Blue);
                series[3]->setName("brightness");
                series[3]->setColor(QColor(1, 1, 1));

                chart->removeAllSeries();
                for (const auto& s : series) {
                    chart->addSeries(s);
                }

                while (!chart->axes().empty()) {
                    chart->removeAxis(chart->axes().back());
                }

                auto xAxis = std::make_unique<QValueAxis>();
                xAxis->setMin(mHistogram->minValue);
                xAxis->setMax(mHistogram->maxValue);
                xAxis->setLabelsBrush(QColorConstants::White);
                chart->addAxis(xAxis.release(), Qt::AlignBottom);

                auto yAxis = std::make_unique<QValueAxis>();
                yAxis->setMin(0.0);
                yAxis->setMax(mHistogram->GetMaxBinValue());
                yAxis->setLabelsBrush(QColorConstants::White);
                chart->addAxis(yAxis.release(), Qt::AlignLeft);

                if (auto legend = chart->legend()) {
                    auto markers = legend->markers();
                    for (size_t i = 0; i < markers.size(); ++i) {
                        connect(markers.at(i), &QLegendMarker::clicked, this, std::bind(&HistogramWidget::onMarkerClicked, this, markers.at(i)));
                        markers.at(i)->setLabelBrush(QColorConstants::White);
                    }
                }
                mChartView->update();
            }
            mIsValid = true;
        }
    }
}

void HistogramWidget::onMarkerClicked(QLegendMarker* marker) const
{
    if (auto series = dynamic_cast<QLineSeries*>(marker->series())) {
        if (series->opacity()) {
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

