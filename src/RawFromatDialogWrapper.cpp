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

#include "RawFormatDialogWrapper.h"

#include <QMutexLocker>

std::pair<bool, RawFormat> RawFromatDialogWrapper::showDialog()
{
    std::unique_ptr<RawFromatDialogWrapper> wrapper;
    wrapper.reset(new RawFromatDialogWrapper());
    wrapper->mDialogFactory = [](QWidget* parent) { return new RawFormatDialog(parent); };
    wrapper->moveToThread(QApplication::instance()->thread());

    {
        QMutexLocker lock(&wrapper->mMutex);
        QMetaObject::invokeMethod(wrapper.get(), &RawFromatDialogWrapper::start, Qt::ConnectionType::QueuedConnection);

        while (!wrapper->mReady) {
            wrapper->mReadyCondition.wait(&wrapper->mMutex);
        }
    }

    return wrapper->mResultValue;
}

RawFromatDialogWrapper::RawFromatDialogWrapper(QObject *parent) 
    : QObject(parent)
{ }

RawFromatDialogWrapper::~RawFromatDialogWrapper()
{
    if (mDialogWidget) {
        QMetaObject::invokeMethod(mDialogWidget, &RawFormatDialog::close, Qt::ConnectionType::QueuedConnection);
    }
}

void RawFromatDialogWrapper::start()
{
    mDialogWidget = mDialogFactory(nullptr);
    connect(mDialogWidget, &RawFormatDialog::result, this, &RawFromatDialogWrapper::onResult);
    mDialogWidget->show();
}

void RawFromatDialogWrapper::onResult(bool success, RawFormat format)
{
    mResultValue = std::make_pair(success, format);
    {
        QMutexLocker lock(&mMutex);
        mReady = true;
        mReadyCondition.notify_all();
    }
}
