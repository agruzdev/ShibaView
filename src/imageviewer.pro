QT += widgets

TARGET = ShibaView

HEADERS       = \
    ViewerApplication.h \
    ImageLoader.h \
    CanvasWidget.h \
    ZoomController.h \
    TextWidget.h \
    ImageInfo.h \
    Image.h \
    Global.h \
    MenuWidget.h \
    ImageSource.h \
    BitmapSource.h \
    MultiBitmapsource.h \
    UniqueTick.h \
    EnumArray.h \
    ImageProcessor.h \
    FreeImageExt.h \
    ImageInfo.h \
    ImageDescription.h \
    Player.h

SOURCES       = \
    main.cpp \
    ViewerApplication.cpp \
    ImageLoader.cpp \
    CanvasWidget.cpp \
    ZoomController.cpp \
    TextWidget.cpp \
    Image.cpp \
    Global.cpp \
    MenuWidget.cpp \
    BitmapSource.cpp \
    MultiBitmapsource.cpp \
    ImageSource.cpp \
    UniqueTick.cpp \
    ImageProcessor.cpp \
    FreeImageExt.cpp \
    ImageDescription.cpp \
    Player.cpp

RC_FILE = resources.rc

INCLUDEPATH += ../3rdParty/freeimage/include
LIBS += ../3rdParty/freeimage/lib/FreeImage.lib
DEFINES += FREEIMAGE_COLORORDER=FREEIMAGE_COLORORDER_RGB

RESOURCES += \
    assets.qrc

FORMS +=
