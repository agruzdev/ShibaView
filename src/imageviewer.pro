QT += widgets

HEADERS       = imageviewer.h
SOURCES       = imageviewer.cpp \
                main.cpp

INCLUDEPATH += ../3rdParty/freeimage/include
LIBS += ../3rdParty/freeimage/lib/FreeImage.lib
DEFINES += FREEIMAGE_COLORORDER=FREEIMAGE_COLORORDER_RGB
