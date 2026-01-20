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
#include <QHBoxLayout>

#include "Controls.h"
#include "Global.h"
#include "TextWidget.h"
#include "FreeImage.h"


namespace
{
    QString MakeDependencyString(const FIDEPENDENCY* dep, uint32_t depth = 1)
    {
        QString res{};
        if (dep) {
            res += "  ";
            for (uint32_t d = 0; d < depth; ++d) {
                res += "- ";
            }

            if (dep->fullVersion) {
                res += QString(dep->fullVersion);
            }
            else {
                if (dep->name) {
                    res += QString(dep->name) + "  ";
                }
                else {
                    res += "N/A";
                }
                res += " v" + QString::number(dep->majorVersion) + "." + QString::number(dep->minorVersion);
            }

            if (depth <= 1 && dep->type == FIDEP_DYNAMIC) {
                res += " (External DLL)";
            }
        }
        return res;
    }

} // namespace


AboutWidget::AboutWidget(QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::WindowCloseButtonHint | Qt::MSWindowsOwnDC);
    //setAttribute(Qt::WA_DeleteOnClose);

    setWindowTitle(Global::makeTitle("About"));

    auto layout = new QHBoxLayout(this);

    // Version info
    {
        auto versText = new TextWidget(this, {}, 11, 0.8);
        versText->setPaddings(4, 0, 2, 0);

        QVector<QString> textLines;
        textLines.emplace_back("Version: " + QString::number(Global::kVersionMajor) + "." + QString::number(Global::kVersionMinor));
        textLines.emplace_back("Copyright 2018-2026 " + Global::kOrganizationName);
        textLines.emplace_back("");
        textLines.emplace_back("Dependencies:");
        textLines.emplace_back("  Qt v" + QString(qVersion()));
        textLines.emplace_back("  FreeImageRe v" + QString(FreeImageRe_GetVersion()));
        for (uint32_t depIdx = 0; depIdx < FreeImage_GetDependenciesCount(); ++depIdx) {
            if (const auto* depInfo = FreeImage_GetDependencyInfo(depIdx)) {
                textLines.emplace_back(MakeDependencyString(depInfo));
                for (auto nextInfo = depInfo->next; nextInfo != nullptr; nextInfo = nextInfo->next) {
                    textLines.emplace_back(MakeDependencyString(nextInfo, /*depth=*/2));
                }
            }
        }

        versText->setText(textLines);
        versText->setFixedSize(400, 90 + textLines.size() * 16);
        layout->addWidget(versText, 0, Qt::AlignTop);
    }

    // Controls info
    {
        auto ctrlText = new TextWidget(this, {}, 11, 0.8);
        ctrlText->setPaddings(4, 0, 2, 0);

        QVector<QString> textLines;
        textLines.emplace_back("Controls:");
        for (const auto& [action, keys] : Controls::getInstance().printControls()) {
            textLines.emplace_back("- " + action + " | " + keys);
        }

        ctrlText->setColumnSeperator('|');
        ctrlText->appendColumnOffset(250);
        ctrlText->setText(textLines);
        ctrlText->setFixedSize(450, 90 + textLines.size() * 16);
        layout->addWidget(ctrlText, 0, Qt::AlignTop);
    }

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
