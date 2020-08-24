/**
 * @file
 *
 * Copyright 2018-2019 Alexey Gruzdev
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

#include "RawFormatDialog.h"
#include "ui_RawFormatDialog.h"
#include <QIntValidator>

RawFormatDialog::RawFormatDialog(QWidget *parent) 
    : QWidget(parent)
{
    qRegisterMetaType<RawFormat>("RawFormat");

    mUi = std::make_unique<Ui::RawFormatDialog>();
    mUi->setupUi(this);

    setWindowTitle("Raw file format");
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);
    setAttribute(Qt::WA_QuitOnClose, false);

    const auto buttonOk = findChild<QPushButton*>("buttonOk");
    connect(buttonOk, &QPushButton::clicked, this, &RawFormatDialog::onOk);

    const auto buttonCancel = findChild<QPushButton*>("buttonCancel");
    connect(buttonCancel, &QPushButton::clicked, this, &RawFormatDialog::onCancel);

    mComboColorSpace = findChild<QComboBox*>("comboColor");
    for (std::underlying_type_t<RawPixelColor> i = 0; i < static_cast<std::underlying_type_t<RawPixelColor>>(RawPixelColor::length_); ++i) {
        mComboColorSpace->addItem(toQString(static_cast<RawPixelColor>(i)));
    }

    mComboPixelType = findChild<QComboBox*>("comboType");
    for (std::underlying_type_t<RawDataType> i = 0; i < static_cast<std::underlying_type_t<RawDataType>>(RawDataType::length_); ++i) {
        mComboPixelType->addItem(toQString(static_cast<RawDataType>(i)));
    }

    mEditWidth = findChild<QLineEdit*>("lineEditWidth");
    mEditWidth->setValidator(new QIntValidator(1, std::numeric_limits<int>::max(), this));
    mEditWidth->setText("1");

    mEditHeight = findChild<QLineEdit*>("lineEditHeight");
    mEditHeight->setValidator(new QIntValidator(1, std::numeric_limits<int>::max(), this));
    mEditHeight->setText("1");
}

RawFormatDialog::~RawFormatDialog() = default;

void RawFormatDialog::closeEvent(QCloseEvent* /*event*/)
{
    if (!mGotResult) {
        mGotResult = true;
        cancel();
    }
    deleteLater();
}

void RawFormatDialog::onOk()
{
    if (!mGotResult) {
        if (ok()) {
            mGotResult = true;
        }
    }
}

void RawFormatDialog::onCancel()
{
    if (!mGotResult) {
        mGotResult = true;
        cancel();
    }
}

bool RawFormatDialog::ok()
{
    if (mEditWidth->hasAcceptableInput() && mEditHeight->hasAcceptableInput()) {
        RawFormat format{};
        format.colorSpace = static_cast<RawPixelColor>(mComboColorSpace->currentIndex());
        format.dataType  = static_cast<RawDataType>(mComboPixelType->currentIndex());
        format.width      = static_cast<uint32_t>(mEditWidth->text().toInt());
        format.height     = static_cast<uint32_t>(mEditHeight->text().toInt());
        //format.lineStride = 0;
        emit result(true, format);
        return true;
    }
    return false;
}

void RawFormatDialog::cancel()
{
    RawFormat format{};
    emit result(false, format);
}
