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
#include "ViewerApplication.h"

int main(int argc, char *argv[])
{
    try {
        if(argc < 2) {
            throw std::runtime_error("Wrong args");
        }
        const auto t = std::chrono::steady_clock::now();

        QApplication app{argc, argv};
        QApplication::setOrganizationName("Alexey Gruzdev");
        QApplication::setApplicationName("ShibaView");

        ViewerApplication viewer(t);
        const QString input = QApplication::arguments().at(1);
        viewer.open(input);
        viewer.loadImageAsync(input);

#ifdef _MSC_VER
        std::cout << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - t).count() / 1e3 << std::endl;
#endif
        return QApplication::exec();
    }
    catch(...){
        return -1;
    }
}
