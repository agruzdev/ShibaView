cmake_minimum_required(VERSION 3.10)

project(ShibaView)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

get_target_property(Qt5Core_BINARY_DEBUG Qt5::Core IMPORTED_LOCATION_DEBUG)
get_target_property(Qt5Core_BINARY_RELEASE Qt5::Core IMPORTED_LOCATION_RELEASE)
get_target_property(Qt5Core_BINARY Qt5::Core IMPORTED_LOCATION)

get_target_property(Qt5Gui_BINARY_DEBUG Qt5::Gui IMPORTED_LOCATION_DEBUG)
get_target_property(Qt5Gui_BINARY_RELEASE Qt5::Gui IMPORTED_LOCATION_RELEASE)
get_target_property(Qt5Gui_BINARY Qt5::Gui IMPORTED_LOCATION)

get_target_property(Qt5Widgets_BINARY_DEBUG Qt5::Widgets IMPORTED_LOCATION_DEBUG)
get_target_property(Qt5Widgets_BINARY_RELEASE Qt5::Widgets IMPORTED_LOCATION_RELEASE)
get_target_property(Qt5Widgets_BINARY Qt5::Widgets IMPORTED_LOCATION)

get_target_property(QWindowsIntegrationPlugin_BINARY_DEBUG Qt5::QWindowsIntegrationPlugin IMPORTED_LOCATION_DEBUG)
get_target_property(QWindowsIntegrationPlugin_BINARY_RELEASE Qt5::QWindowsIntegrationPlugin IMPORTED_LOCATION_RELEASE)
get_target_property(QWindowsIntegrationPlugin_BINARY Qt5::QWindowsIntegrationPlugin IMPORTED_LOCATION)


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

target_compile_features(ShibaView PRIVATE cxx_std_14)

target_link_libraries(ShibaView Qt5::Core Qt5::Gui Qt5::Widgets ${FreeImage_LIBRARY})

install(TARGETS ShibaView DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES ${FreeImage_BINARY} DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES ${CMAKE_SOURCE_DIR}/LICENSE DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES "${CMAKE_SOURCE_DIR}/3rdParty/freeimage/FreeImage License.txt" DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES "${CMAKE_SOURCE_DIR}/fonts/Font License.txt" DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES "${CMAKE_SOURCE_DIR}/icons/Icon License.txt" DESTINATION ${CMAKE_INSTALL_PREFIX})
if(MSVC)
    get_filename_component(Qt5_binary_dir ${Qt5Core_BINARY_RELEASE} DIRECTORY)
    set_target_properties(ShibaView PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    set_target_properties(ShibaView PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=${CMAKE_SOURCE_DIR}/3rdParty/freeimage/lib;${Qt5_binary_dir};")
    install(FILES ${Qt5Core_BINARY_DEBUG} ${Qt5Gui_BINARY_DEBUG} ${Qt5Widgets_BINARY_DEBUG} DESTINATION ${CMAKE_INSTALL_PREFIX} CONFIGURATIONS DEBUG)
    install(FILES ${QWindowsIntegrationPlugin_BINARY_DEBUG} DESTINATION ${CMAKE_INSTALL_PREFIX}/platforms CONFIGURATIONS DEBUG)
    install(FILES ${Qt5Core_BINARY_RELEASE} ${Qt5Gui_BINARY_RELEASE} ${Qt5Widgets_BINARY_RELEASE} DESTINATION ${CMAKE_INSTALL_PREFIX} CONFIGURATIONS RELEASE)
    install(FILES ${QWindowsIntegrationPlugin_BINARY_RELEASE} DESTINATION ${CMAKE_INSTALL_PREFIX}/platforms CONFIGURATIONS RELEASE)
else()
    install(FILES ${Qt5Core_BINARY} ${Qt5Gui_BINARY} ${Qt5Widgets_BINARY} DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(FILES ${QWindowsIntegrationPlugin_BINARY} DESTINATION ${CMAKE_INSTALL_PREFIX}/platforms)
endif()


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

    set_target_properties(ShibaThumbnail PROPERTIES PREFIX "")

    target_compile_features(ShibaThumbnail PRIVATE cxx_std_14)

    target_link_libraries(ShibaThumbnail Shlwapi Qt5::Core ${FreeImage_LIBRARY})

    install(TARGETS ShibaThumbnail DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()
