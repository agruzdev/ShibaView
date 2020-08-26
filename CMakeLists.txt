cmake_minimum_required(VERSION 3.10)

project(ShibaView)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/3rdParty/freeimage/include)
set(FreeImage_LIBRARY "${CMAKE_SOURCE_DIR}/3rdParty/freeimage/lib/FreeImage.lib")
set(FreeImage_BINARY  "${CMAKE_SOURCE_DIR}/3rdParty/freeimage/lib/FreeImage.dll")

add_definitions(/DFREEIMAGE_COLORORDER=FREEIMAGE_COLORORDER_RGB)

set(app_sources
    ./src/ViewerApplication.h
    ./src/ViewerApplication.cpp
    ./src/ImageLoader.h
    ./src/ImageLoader.cpp
    ./src/CanvasWidget.h
    ./src/CanvasWidget.cpp
    ./src/ZoomController.h
    ./src/ZoomController.cpp
    ./src/TextWidget.h
    ./src/TextWidget.cpp
    ./src/ImageInfo.h
    ./src/Image.h
    ./src/Image.cpp
    ./src/Global.h
    ./src/Global.cpp
    ./src/MenuWidget.h
    ./src/MenuWidget.cpp
    ./src/ImageSource.h
    ./src/ImageSource.cpp
    ./src/BitmapSource.h
    ./src/BitmapSource.cpp
    ./src/MultiBitmapsource.h
    ./src/MultiBitmapsource.cpp
    ./src/UniqueTick.h
    ./src/UniqueTick.cpp
    ./src/EnumArray.h
    ./src/ImageProcessor.h
    ./src/ImageProcessor.cpp
    ./src/FreeImageExt.h
    ./src/FreeImageExt.cpp
    ./src/ImageInfo.h
    ./src/ImageDescription.h
    ./src/ImageDescription.cpp
    ./src/Player.h
    ./src/Player.cpp
    ./src/main.cpp
)

set(app_resources
    ./src/resources.rc
    ./src/assets.qrc
)

add_executable(ShibaView WIN32 ${app_sources} ${app_resources})

target_link_libraries(ShibaView Qt5::Core Qt5::Gui Qt5::Widgets ${FreeImage_LIBRARY})

add_custom_command(TARGET ShibaView POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${FreeImage_BINARY} $<TARGET_FILE_DIR:ShibaView> COMMENT "Copy the FreeImage library.")

if(WIN32)
    set(thumbnails_sources
        ./src/FreeImageExt.h
        ./src/FreeImageExt.cpp
        ./src/ImageSource.h
        ./src/ImageSource.cpp
        ./src/BitmapSource.h
        ./src/BitmapSource.cpp
        ./src/MultiBitmapsource.h
        ./src/MultiBitmapsource.cpp
        ./src/Player.h
        ./src/Player.cpp
        ./src/Global.h
        ./src/Global.cpp
        ./src/thumbnails/WindowsThumbnailProvider.h
        ./src/thumbnails/WindowsThumbnailProvider.cpp
        ./src/thumbnails/WindowsThumbnailService.cpp
        ./src/thumbnails/WindowsThumbnailService.def
    )

    add_library(ShibaThumbnail SHARED ${thumbnails_sources})

    target_link_libraries(ShibaThumbnail Shlwapi Qt5::Core ${FreeImage_LIBRARY})
endif()
