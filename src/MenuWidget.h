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

#ifndef MENUWIDGET_H
#define MENUWIDGET_H

#include <QWidget>
#include <QLayout>

class TextWidget;

class MenuWidget
    : public QWidget
{
    Q_OBJECT

signals:
    void toggled(bool checked) const;
    void activated() const;

public slots:
    void onActionToggled(bool checked);

public:
    MenuWidget(QWidget *parent = nullptr);
    MenuWidget(const QString &text, QWidget *parent = nullptr);

    ~MenuWidget();

    QSize minimumSizeHint() const Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent* e) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent* evt) Q_DECL_OVERRIDE;

private:
    void init();

    QString text = "";
    bool checked = false;

    QLayout* mLayout;
    TextWidget* mBulletWidget;
    TextWidget* mTextWidget;
};




#endif // MENUWIDGET_H
