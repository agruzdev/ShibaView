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
#include <QKeyEvent>
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
    textLines.push_back("Copyright 2018-2020 " + Global::kOrganizationName);
    textLines.push_back("");
    textLines.push_back("Using:");
    textLines.push_back("  Qt " + QString(qVersion()));
    textLines.push_back("  FreeImage " + QString(FreeImage_GetVersion()));
    textLines.push_back("");
    textLines.push_back("");
    textLines.push_back("Controls:");
    textLines.push_back("  F1         | -  Show this page");
    textLines.push_back("  F2         | -  Show EXIF data");
    textLines.push_back("  Left       | -  Previous image");
    textLines.push_back("  Right      | -  Next image");
    textLines.push_back("  Home       | -  First image in directory");
    textLines.push_back("  End        | -  Last image in directory");
    textLines.push_back("  Plus       | -  Zoom in");
    textLines.push_back("  Minus      | -  Zoom out");
    textLines.push_back("  Asterisk   | -  Toggle 100% zoom / fit screen");
    textLines.push_back("  Space      | -  Pause animation playback");
    textLines.push_back("  PageDown   | -  Previous animation frame");
    textLines.push_back("  PageUp     | -  Next animation frame");
    textLines.push_back("  Ctrl+R     | -  Reload the current image");
    textLines.push_back("  Ctrl+Up    | -  Toggle rotation 0" UTF8_DEGREE);
    textLines.push_back("  Ctrl+Right | -  Toggle rotation 90" UTF8_DEGREE);
    textLines.push_back("  Ctrl+Left  | -  Toggle rotation 270" UTF8_DEGREE);
    textLines.push_back("  Ctrl+Down  | -  Toggle rotation 180" UTF8_DEGREE);
    textLines.push_back("  Ctrl+I     | -  Color picker mode");
    textLines.push_back("  Esc        | -  Quit");
    text->setColumnSeperator('|');
    text->appendColumnOffset(110);
    text->setText(textLines);

    setFixedSize(350, 530);

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
