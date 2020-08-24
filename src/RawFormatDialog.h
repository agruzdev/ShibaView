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

#ifndef RAWFORMATDIALOG_H
#define RAWFORMATDIALOG_H

#include <memory>
#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include "RawFormat.h"

namespace Ui
{
    class RawFormatDialog;
}

class RawFormatDialog
    : public QWidget
{
    Q_OBJECT
public:
    explicit
    RawFormatDialog(QWidget *parent = nullptr);

    ~RawFormatDialog() override;

signals:
    void result(bool success, RawFormat value);

public slots:
    void onOk();
    void onCancel();

protected:
    void closeEvent(QCloseEvent* event) Q_DECL_OVERRIDE;

private:
    bool ok();
    void cancel();

    std::unique_ptr<Ui::RawFormatDialog> mUi;
    bool mGotResult = false;

    QComboBox* mComboColorSpace;
    QComboBox* mComboPixelType;
    QLineEdit* mEditWidth;
    QLineEdit* mEditHeight;
};

#endif // RAWFORMATDIALOG_H
