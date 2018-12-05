#include <iostream>

#include <QApplication>
#include <QCommandLineParser>

#include "imageviewer.h"

int main(int argc, char *argv[])
{
    if(argc < 2) {
        return 1;
    }
    auto t = std::chrono::steady_clock::now();

    QApplication app(argc, argv);
    app.setOrganizationName("ShibaSoft");
    app.setApplicationName("ShibaView");

    GuiWidget widget(t);
    widget.setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    widget.loadImageAsync(QString(argv[1]));

    std::cout << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - t).count() / 1e3 << std::endl;
    return app.exec();
}
