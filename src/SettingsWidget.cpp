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

#include "SettingsWidget.h"

#include <QGridLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QValidator>
#include "Global.h"
#include "TextWidget.h"
#include "Global.h"


SettingsWidget::SettingsWidget()
    : QWidget(nullptr)
{
    constexpr qreal kTitleFontSize = 16.0;
    constexpr qreal kLabelFontSize = 12.0;

    mSettings = Global::getSettings(Global::SettingsGroup::eGlobal);
    assert(mSettings);

    setWindowFlags(Qt::WindowCloseButtonHint | Qt::MSWindowsOwnDC);

    auto layout = new QGridLayout(this);
    layout->setSpacing(24);

    //
    auto title = std::make_unique<TextWidget>(nullptr, QColorConstants::Black, kTitleFontSize);
    title->setText("Settings:");
    layout->addWidget(title.release(), 0, 0);

    //
    auto label1 = std::make_unique<TextWidget>(nullptr, QColorConstants::Black, kLabelFontSize);
    label1->setText("Background color");

    auto field1 = std::make_unique<QLineEdit>(nullptr);
    field1->setText(mSettings->value(Global::kParamBackgroundKey, Global::kParamBackgroundDefault).toString());
    field1->setValidator(new QRegularExpressionValidator(QRegularExpression("#[0-9a-fA-F]{6}")));
    mEditBackground = field1.get();

    layout->addWidget(label1.release(), 1, 0);
    layout->addWidget(field1.release(), 1, 1);


    //
    auto buttonApply = std::make_unique<QPushButton>("Apply");
    connect(buttonApply.get(), &QPushButton::clicked, this, &SettingsWidget::onApply);
    layout->addWidget(buttonApply.release(), 2, 1);
}

SettingsWidget::~SettingsWidget() = default;

void SettingsWidget::onApply()
{
    bool wasChanged = false;
    if (mEditBackground && mEditBackground->hasAcceptableInput()) {
        mSettings->setValue(Global::kParamBackgroundKey, mEditBackground->text());
        wasChanged = true;
    }
    if (wasChanged) {
        emit changed();
    }
}
