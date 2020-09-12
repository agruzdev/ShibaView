/**
 * @file
 *
 * Copyright 2018-2020 Alexey Gruzdev
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

#include "AboutWidget.h"

#include <mutex>
#include <QColor>
#include <QString>
#include "Global.h"
#include "TextWidget.h"

namespace
{
    struct AboutWidgetStaticContext
    {
        bool isCreated = false;
        std::mutex mutex;
    };

    AboutWidgetStaticContext gAboutWidgetStaticContext{};
}

void AboutWidget::showInstance()
{
    std::lock_guard<std::mutex> lock(gAboutWidgetStaticContext.mutex);
    if (!gAboutWidgetStaticContext.isCreated) {
        gAboutWidgetStaticContext.isCreated = true;
        new AboutWidget();
    }
}

AboutWidget::AboutWidget()
    : QWidget(nullptr)
{
    setWindowFlags(Qt::WindowCloseButtonHint | Qt::MSWindowsOwnDC);
    setAttribute(Qt::WA_DeleteOnClose);

    setWindowTitle(Global::kApplicationName + " - About");

    setFixedSize(300, 128);

    auto text = new TextWidget(this, Qt::black, 11);
    text->setPaddings(8, 0, 4, 0);

    QVector<QString> textLines;
    textLines.push_back("Version: " + QString::number(Global::kVersionMajor) + "." + QString::number(Global::kVersionMinor));
    textLines.push_back("Copyright 2018-2020 " + Global::kOrganizationName);
    textLines.push_back("");
    textLines.push_back("Built with Qt" + QString::number(QT_VERSION_MAJOR) + "." + QString::number(QT_VERSION_MINOR));
    text->setText(textLines);

    update();
    show();
}

AboutWidget::~AboutWidget()
{
    std::lock_guard<std::mutex> lock(gAboutWidgetStaticContext.mutex);
    gAboutWidgetStaticContext.isCreated = false;
}

