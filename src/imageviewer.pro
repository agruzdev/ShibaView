QT += widgets

TARGET = ShibaView

HEADERS       = \
    ViewerApplication.h \
    ImageLoader.h \
    CanvasWidget.h \
    ZoomController.h \
    TextWidget.h \
    ImageInfo.h

SOURCES       = \
    main.cpp \
    ViewerApplication.cpp \
    ImageLoader.cpp \
    CanvasWidget.cpp \
    ZoomController.cpp \
    TextWidget.cpp

RC_FILE = resources.rc

INCLUDEPATH += ../3rdParty/freeimage/include
LIBS += ../3rdParty/freeimage/lib/FreeImage.lib
DEFINES += FREEIMAGE_COLORORDER=FREEIMAGE_COLORORDER_RGB

RESOURCES += \
    assets.qrc
