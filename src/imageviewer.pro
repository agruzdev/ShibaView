QT += widgets

TARGET = ShibaView

HEADERS       = \
    ViewerApplication.h \
    ImageLoader.h \
    CanvasWidget.h

SOURCES       = \
    main.cpp \
    ViewerApplication.cpp \
    ImageLoader.cpp \
    CanvasWidget.cpp

INCLUDEPATH += ../3rdParty/freeimage/include
LIBS += ../3rdParty/freeimage/lib/FreeImage.lib
DEFINES += FREEIMAGE_COLORORDER=FREEIMAGE_COLORORDER_RGB
