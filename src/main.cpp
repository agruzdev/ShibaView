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

#include <iostream>
#include <chrono>

#include <QApplication>
#include <QFileDialog>
#include <QSettings>
#include <QFileInfo>
#include "ViewerApplication.h"

namespace
{
    const QString kSettingsLoadDir = "application/load_directory";
}

int main(int argc, char *argv[])
try
{
    const auto t = std::chrono::steady_clock::now();
        
    QApplication app{argc, argv};
    QApplication::setOrganizationName("Alexey Gruzdev");
    QApplication::setApplicationName("ShibaView");

    QString input;
    if (argc > 1) {
        input = QApplication::arguments().at(1);
    }
    else {
        QSettings settings;
        input = QFileDialog::getOpenFileName(nullptr, "Open File", settings.value(kSettingsLoadDir, "/").toString(), ViewerApplication::getFileFilter());
        if (!input.isEmpty()) {
            settings.setValue(kSettingsLoadDir, QFileInfo(input).dir().absolutePath());
        }
    }

    if (input.isEmpty()) {
        return 0;
    }

    ViewerApplication viewer(t);
    viewer.open(input);

#ifdef _MSC_VER
    std::cout << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - t).count() / 1e3 << std::endl;
#endif
    return QApplication::exec();
}
catch(...){
    return -1;
}
