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

#ifndef HISTOGRAMWIDGET_H
#define HISTOGRAMWIDGET_H

#include <limits>
#include <QWidget>
#include <QFont>
#include <QRawFont>
#include <QPen>
#include <QBrush>
#include <QGraphicsDropShadowEffect>
#include <QChartView>
#include <QLineSeries>

#include "Image.h"

class Histogram;
class DragCornerWidget;
class Tooltip;

class HistogramWidget 
    : public QWidget
    , public ImageListener
{
    Q_OBJECT
public:
    explicit HistogramWidget(QWidget* parent = nullptr);
    ~HistogramWidget();

    void attachImageSource(QWeakPointer<Image> image);

public slots:
    void onMarkerClicked(QLegendMarker* marker) const;
    void onPointHovered(QLineSeries* series, const QPointF& point, bool state) const;

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

private:
    void mousePressEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent* event) Q_DECL_OVERRIDE;

    void keyPressEvent(QKeyEvent* event) Q_DECL_OVERRIDE;
    void keyReleaseEvent(QKeyEvent* event) Q_DECL_OVERRIDE;

    void onDraggedStart();
    void onDraggedOffset(QPoint offset);
    void onDraggedStop();

    void onStretchedStart();
    void onStretchedOffset(QPoint offset);
    void onStretchedStop();

    void moveSticky(QPoint pos, bool updateStickyFlags = true);
    void updatePositionOnResize();

    void onInvalidated(Image*) Q_DECL_OVERRIDE;

    QRect getParentSpace() const;

    QPoint mCurrentPos{};
    QRect mCurrentGeometry{};

    QWeakPointer<Image> mImageSource;
    std::unique_ptr<Histogram> mHistogram;
    bool mIsValid = false;

    QChartView* mChartView = nullptr;
    DragCornerWidget* mDragCorner = nullptr;
    DragCornerWidget* mStretchCorner = nullptr;

    bool mStickyLeft{ false };
    bool mStickyTop{ false };
    bool mStickyRight{ false };
    bool mStickyBottom{ false };

    std::unique_ptr<Tooltip> mTooltip;
};

#endif // HISTOGRAMWIDGET_H
