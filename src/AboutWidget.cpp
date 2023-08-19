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

#include "AboutWidget.h"

#include <mutex>
#include <QColor>
#include <QString>
#include <QKeyEvent>
#include <QLibrary>
#include "Controls.h"
#include "Global.h"
#include "TextWidget.h"
#include "FreeImage.h"


namespace
{
    struct WidgetStaticContext
    {
        AboutWidget* instance = nullptr;
        std::mutex mutex;
    };

    WidgetStaticContext gAboutWidgetStaticContext{};
}

AboutWidget& AboutWidget::getInstance()
{
    std::lock_guard<std::mutex> lock(gAboutWidgetStaticContext.mutex);
    if (!gAboutWidgetStaticContext.instance) {
        gAboutWidgetStaticContext.instance = new AboutWidget();
    }
    return *gAboutWidgetStaticContext.instance;
}

AboutWidget::AboutWidget()
    : QWidget(nullptr)
{
    setWindowFlags(Qt::WindowCloseButtonHint | Qt::MSWindowsOwnDC);
    setAttribute(Qt::WA_DeleteOnClose);

    setWindowTitle(Global::kApplicationName + " - About");

    auto text = new TextWidget(this, Qt::black, 11, 0.8);
    text->setPaddings(8, 0, 4, 0);

    QVector<QString> textLines;
    textLines.push_back("Version: " + QString::number(Global::kVersionMajor) + "." + QString::number(Global::kVersionMinor));
    textLines.push_back("Copyright 2018-2023 " + Global::kOrganizationName);
    textLines.push_back("");
    textLines.push_back("Using:");
    textLines.push_back("  Qt v" + QString(qVersion()));
    textLines.push_back("  FreeImageRe v" + QString(FreeImageRe_GetVersion()) + " (" + QString::number(FREEIMAGE_MAJOR_VERSION) + "." + QString::number(FREEIMAGE_MINOR_VERSION) + ")");
    textLines.push_back("");
    textLines.push_back("");
    textLines.push_back("Controls:");
    for (const auto& [action, keys] : Controls::getInstance().printControls()) {
        textLines.push_back("  " + action + " | " + keys);
    }
    text->setColumnSeperator('|');
    text->appendColumnOffset(225);
    text->setText(textLines);

    setFixedSize(400, 90 + textLines.size() * 16);

    update();
    show();
}

AboutWidget::~AboutWidget()
{
    std::lock_guard<std::mutex> lock(gAboutWidgetStaticContext.mutex);
    gAboutWidgetStaticContext.instance = nullptr;
}

void AboutWidget::popUp()
{
}

void AboutWidget::keyPressEvent(QKeyEvent *event)
{
    QWidget::keyPressEvent(event);
    if (event->key() == Qt::Key_Escape) {
        close();
    }
}
