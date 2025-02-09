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


AboutWidget::AboutWidget(QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::WindowCloseButtonHint | Qt::MSWindowsOwnDC);
    //setAttribute(Qt::WA_DeleteOnClose);

    setWindowTitle(Global::makeTitle("About"));

    auto text = new TextWidget(this, Qt::black, 11, 0.8);
    text->setPaddings(8, 0, 4, 0);

    QVector<QString> textLines;
    textLines.emplace_back("Version: " + QString::number(Global::kVersionMajor) + "." + QString::number(Global::kVersionMinor));
    textLines.emplace_back("Copyright 2018-2024 " + Global::kOrganizationName);
    textLines.emplace_back("");
    textLines.emplace_back("Dependencies:");
    textLines.emplace_back("  Qt v" + QString(qVersion()));
    textLines.emplace_back("  FreeImageRe v" + QString(FreeImageRe_GetVersion()) + " (" + QString::number(FREEIMAGE_MAJOR_VERSION) + "." + QString::number(FREEIMAGE_MINOR_VERSION) + ")");
    for (uint32_t depIdx = 0; depIdx < FreeImage_GetDependenciesCount(); ++depIdx) {
        if (const auto* depInfo = FreeImage_GetDependencyInfo(depIdx)) {
            textLines.emplace_back("   - " + QString(depInfo->name) + "  v" + QString(depInfo->fullVersion));
            if (depInfo->type == FIDEP_DYNAMIC) {
                textLines.back().append(" (External DLL)");
            }
        }
    }
    textLines.emplace_back("");
    textLines.emplace_back("Controls:");
    for (const auto& [action, keys] : Controls::getInstance().printControls()) {
        textLines.emplace_back("  " + action + " | " + keys);
    }
    textLines.emplace_back("");
    text->setColumnSeperator('|');
    text->appendColumnOffset(300);
    text->setText(textLines);

    setFixedSize(500, 90 + textLines.size() * 16);

    update();
    show();
}

AboutWidget::~AboutWidget() = default;

void AboutWidget::keyPressEvent(QKeyEvent *event)
{
    QWidget::keyPressEvent(event);
    if (event->key() == Qt::Key_Escape) {
        close();
    }
}
