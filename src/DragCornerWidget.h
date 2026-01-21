/**
 * @file
 *
 * Copyright 2018-2026 Alexey Gruzdev
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

#ifndef DRAGCORNERWIDGET_H
#define DRAGCORNERWIDGET_H

#include <QWidget>

class DragCornerWidget 
    : public QWidget
{
    Q_OBJECT
public:
    explicit DragCornerWidget(QWidget* parent, uint32_t size, QColor background = QColorConstants::Green);
    ~DragCornerWidget();

    void setCornerRadius(qreal value = 0.0)
    {
        mCornerRadius = value;
    }

signals:
    void draggingStart();
    void draggingOffset(QPoint offset);
    void draggingStop();

protected:
    void paintEvent(QPaintEvent* event) Q_DECL_OVERRIDE;

private:
    void mousePressEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent* event) Q_DECL_OVERRIDE;

    void keyPressEvent(QKeyEvent* event) Q_DECL_OVERRIDE;
    void keyReleaseEvent(QKeyEvent* event) Q_DECL_OVERRIDE;

    QPoint mClickLocalPos{};
    bool mIsDragged = false;
    qreal mCornerRadius = 0.0;
    QColor mBackgroundColor;

};

#endif // DRAGCORNERWIDGET_H
