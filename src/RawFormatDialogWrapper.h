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

#ifndef RAWFORMATDIALOGWRAPPER_H
#define RAWFORMATDIALOGWRAPPER_H

#include <QApplication>
#include <QMetaObject>
#include <QObject>
#include <QMutex>
#include <QWaitCondition>
#include "RawFormatDialog.h"
#include "RawFormat.h"

class RawFromatDialogWrapper
    : public QObject
{
    Q_OBJECT

    using DialogFactory = std::function<RawFormatDialog*(QWidget*)>;

public:
    ~RawFromatDialogWrapper() Q_DECL_OVERRIDE;

    static
    std::pair<bool, RawFormat> showDialog();

signals:

public slots:
    void start();

    void onResult(bool success, RawFormat format);

private:
    explicit
    RawFromatDialogWrapper(QObject *parent = nullptr);


    DialogFactory mDialogFactory;
    RawFormatDialog* mDialogWidget = nullptr;
    QMutex mMutex;
    QWaitCondition mReadyCondition;
    std::pair<bool, RawFormat> mResultValue;
    bool mReady = false;
};

#endif // RAWFORMATDIALOGWRAPPER_H
