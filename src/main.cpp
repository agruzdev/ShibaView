/**
* ShibaView
*
* The MIT License (MIT)
* Copyright (c) 2018 Alexey Gruzdev
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
