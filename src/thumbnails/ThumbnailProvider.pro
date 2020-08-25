TEMPLATE = lib

TARGET = ShibaThumbnail

HEADERS       = \
    ../FreeImageExt.h \
    ../ImageSource.h \
    ../BitmapSource.h \
    ../MultiBitmapsource.h \
    ../Player.h \
    ../Global.h \
    WindowsThumbnailProvider.h

SOURCES       = \
    ../FreeImageExt.cpp \
    ../BitmapSource.cpp \
    ../MultiBitmapsource.cpp \
    ../ImageSource.cpp \
    ../Player.cpp \
    ../Global.cpp \
    WindowsThumbnailProvider.cpp \
    WindowsThumbnailService.cpp

DEF_FILE = $$absolute_path("WindowsThumbnailService.def")

INCLUDEPATH += ../3rdParty/freeimage/include
LIBS += ../3rdParty/freeimage/lib/FreeImage.lib
LIBS += Shlwapi.lib
DEFINES += FREEIMAGE_COLORORDER=FREEIMAGE_COLORORDER_RGB

FORMS +=
