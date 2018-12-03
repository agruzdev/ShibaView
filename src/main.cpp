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
    GuiWidget widget(std::string(argv[1]), t);
    widget.setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    widget.show();
    std::cout << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - t).count() / 1e3 << std::endl;
    return app.exec();
}
