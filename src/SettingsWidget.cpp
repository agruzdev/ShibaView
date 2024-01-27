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
    constexpr qreal kTitleFontSize = 14.0;
    constexpr qreal kLabelFontSize = 12.0;

    mSettings = Global::getSettings(Global::SettingsGroup::eGlobal);
    assert(mSettings);

    setWindowTitle(Global::kApplicationName + " - Settings");
    setWindowFlags(Qt::WindowCloseButtonHint | Qt::MSWindowsOwnDC);

    auto layout = new QGridLayout(this);
    layout->setSpacing(8);

    //
    auto title = std::make_unique<TextWidget>(nullptr, QColorConstants::Black, kTitleFontSize);
    title->setText("Global");
    layout->addWidget(title.release(), 0, 0);

    //
    auto field1 = std::make_unique<QLineEdit>(nullptr);
    field1->setText(mSettings->value(Global::kParamBackgroundKey, Global::kParamBackgroundDefault).toString());
    field1->setValidator(new QRegularExpressionValidator(QRegularExpression("#[0-9a-fA-F]{6}")));
    mEditBackgroundColor = field1.get();

    //
    auto field2 = std::make_unique<QLineEdit>(nullptr);
    field2->setText(mSettings->value(Global::kParamTextColorKey, Global::kParamTextColorDefault).toString());
    field2->setValidator(new QRegularExpressionValidator(QRegularExpression("#[0-9a-fA-F]{6}")));
    mEditTextColor = field2.get();

    //
    auto field3 = std::make_unique<QCheckBox>(nullptr);
    field3->setChecked(mSettings->value(Global::kParamShowCloseButtonKey, Global::kParamShowCloseButtonDefault).toBool());
    mShowCloseButton = field3.get();

    //
    uint32_t lineIndex = 1;
    auto appendOption = [&](QString labelText, auto& edit) mutable {
        auto label = std::make_unique<TextWidget>(nullptr, QColorConstants::Black, kLabelFontSize);
        label->setText(labelText);
        layout->addWidget(label.release(), lineIndex, 0);
        layout->addWidget(edit.release(),  lineIndex, 1);
        ++lineIndex;
    };
    appendOption("Background color", field1);
    appendOption("Text color", field2);
    appendOption("Show Close button", field3);

    //
    auto buttonApply = std::make_unique<QPushButton>("Apply");
    connect(buttonApply.get(), &QPushButton::clicked, this, &SettingsWidget::onApply);
    layout->addWidget(buttonApply.release(), lineIndex, 1);
}

SettingsWidget::~SettingsWidget() = default;

void SettingsWidget::onApply()
{
    bool wasChanged = false;
    if (mEditBackgroundColor && mEditBackgroundColor->hasAcceptableInput()) {
        mSettings->setValue(Global::kParamBackgroundKey, mEditBackgroundColor->text());
        wasChanged = true;
    }
    if (mEditTextColor && mEditTextColor->hasAcceptableInput()) {
        mSettings->setValue(Global::kParamTextColorKey, mEditTextColor->text());
        wasChanged = true;
    }
    if (mShowCloseButton) {
        mSettings->setValue(Global::kParamShowCloseButtonKey, mShowCloseButton->isChecked());
        wasChanged = true;
    }
    if (wasChanged) {
        emit changed();
    }
}
