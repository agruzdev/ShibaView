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

#include "MenuWidget.h"

#include <QPainter>
#include <QStyleOptionMenuItem>
#include <QHBoxLayout>

#include "TextWidget.h"

namespace
{
    Q_DECL_CONSTEXPR uint32_t MENU_MARGIN   = 6;
    Q_DECL_CONSTEXPR uint32_t MENU_HPADDING = 16;
    Q_DECL_CONSTEXPR uint32_t MENU_BULLET_WIDTH = 8;
}

#define UTF8_CHECK_SYMBOL "\xe2\x80\xa2"

MenuWidget::MenuWidget(QWidget* parent)
    : QWidget(parent)
{
    init();
}

MenuWidget::MenuWidget(const QString & text, QWidget* parent)
    : QWidget(parent), text(text)
{
    init();
}

MenuWidget::~MenuWidget() = default;

void MenuWidget::init()
{
    setMouseTracking(true);  //so we get paint updates

    mTextWidget = new TextWidget(this, Qt::black, 12, 0.9);
    mTextWidget->setText(text);
    mTextWidget->setMouseTracking(true);

    mBulletWidget = new TextWidget(this, Qt::black, 12, 0.9);
    mBulletWidget->setText(UTF8_CHECK_SYMBOL);
    mBulletWidget->setMouseTracking(true);
    mBulletWidget->setFixedSize(MENU_BULLET_WIDTH, mTextWidget->textHeight());

    mLayout = new QHBoxLayout();
    mLayout->setContentsMargins(0, 0, 0, 0);
    mLayout->addWidget(mBulletWidget);
    mLayout->addWidget(mTextWidget);
    setLayout(mLayout);
}

QSize MenuWidget::minimumSizeHint() const
{
    return QSize(MENU_MARGIN + MENU_BULLET_WIDTH + mTextWidget->textWidth() + MENU_HPADDING, mTextWidget->textHeight());
}

void MenuWidget::paintEvent(QPaintEvent* /*e*/)
{
    // ToDo: There should be a better way to draw selection
    if (rect().contains(mapFromGlobal(QCursor::pos()))) {
        QPainter p(this);
        QStyleOptionMenuItem opt;
        opt.type = QStyleOption::SO_MenuItem;
        opt.initFrom(this);
        opt.checkType = QStyleOptionMenuItem::Exclusive;
        opt.menuItemType = QStyleOptionMenuItem::Normal;

        opt.state |= QStyle::State_Selected;

        style()->drawControl(QStyle::CE_MenuItem, &opt, &p, this);
    }

    if (checked) {
        mBulletWidget->setVisible(true);
        mLayout->setContentsMargins(MENU_MARGIN, 0, 0, 0);
    }
    else {
        // ToDo: There should be a better way to compute offset
        mBulletWidget->setVisible(false);
        mLayout->setContentsMargins(2 * MENU_MARGIN + MENU_BULLET_WIDTH, 0, 0, 0);
    }
}

void MenuWidget::mouseReleaseEvent(QMouseEvent* evt)
{
    QWidget::mouseReleaseEvent(evt);
    emit toggled(true);
}

void MenuWidget::onActionToggled(bool checked)
{
    this->checked = checked;
}

