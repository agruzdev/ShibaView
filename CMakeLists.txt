#
# Copyright 2018-2023 Alexey Gruzdev
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#   http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.16)

project(ShibaView)

set(SHIBA_VERSION_MAJOR 1)
set(SHIBA_VERSION_MINOR 11)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if (Qt6_DIR)
    set(Qt6Core_DIR ${Qt6_DIR}Core)
    set(Qt6CoreTools_DIR ${Qt6_DIR}CoreTools)
    set(Qt6EntryPoint_DIR ${Qt6_DIR}EntryPoint)
    set(Qt6Gui_DIR ${Qt6_DIR}Gui)
    set(Qt6GuiTools_DIR ${Qt6_DIR}GuiTools)
    set(Qt6OpenGL_DIR ${Qt6_DIR}OpenGL)
    set(Qt6Svg_DIR ${Qt6_DIR}Svg)
    set(Qt6Charts_DIR ${Qt6_DIR}Charts)
    set(Qt6OpenGL_DIR ${Qt6_DIR}OpenGL)
    set(Qt6OpenGLWidgets_DIR ${Qt6_DIR}OpenGLWidgets)
    set(Qt6Widgets_DIR ${Qt6_DIR}Widgets)
    set(Qt6WidgetsTools_DIR ${Qt6_DIR}WidgetsTools)
    if(WIN32)
        set(Qt6QWindowsIntegrationPlugin_DIR ${Qt6_DIR}Gui)
    else()
#        set(Qt6QWindowsIntegrationPlugin_DIR ${Qt6_DIR}Gui)
    endif()
endif()

if (WIN32)
    find_package(Qt6 COMPONENTS Core Gui Widgets Svg Charts OpenGL OpenGLWidgets QWindowsIntegrationPlugin REQUIRED)
else()
    find_package(Qt6 COMPONENTS Core Gui Widgets Svg Charts OpenGL OpenGLWidgets REQUIRED)
endif()

get_target_property(Qt5Core_BINARY_DEBUG Qt6::Core IMPORTED_LOCATION_DEBUG)
get_target_property(Qt5Core_BINARY_RELEASE Qt6::Core IMPORTED_LOCATION_RELEASE)

get_target_property(Qt5Gui_BINARY_DEBUG Qt6::Gui IMPORTED_LOCATION_DEBUG)
get_target_property(Qt5Gui_BINARY_RELEASE Qt6::Gui IMPORTED_LOCATION_RELEASE)

get_target_property(Qt5Widgets_BINARY_DEBUG Qt6::Widgets IMPORTED_LOCATION_DEBUG)
get_target_property(Qt5Widgets_BINARY_RELEASE Qt6::Widgets IMPORTED_LOCATION_RELEASE)

get_target_property(Qt5Svg_BINARY_DEBUG Qt6::Svg IMPORTED_LOCATION_DEBUG)
get_target_property(Qt5Svg_BINARY_RELEASE Qt6::Svg IMPORTED_LOCATION_RELEASE)

get_target_property(Qt5Charts_BINARY_DEBUG Qt6::Charts IMPORTED_LOCATION_DEBUG)
get_target_property(Qt5Charts_BINARY_RELEASE Qt6::Charts IMPORTED_LOCATION_RELEASE)

get_target_property(Qt5OpenGL_BINARY_DEBUG Qt6::OpenGL IMPORTED_LOCATION_DEBUG)
get_target_property(Qt5OpenGL_BINARY_RELEASE Qt6::OpenGL IMPORTED_LOCATION_RELEASE)

get_target_property(Qt5OpenGLWidgets_BINARY_DEBUG Qt6::OpenGLWidgets IMPORTED_LOCATION_DEBUG)
get_target_property(Qt5OpenGLWidgets_BINARY_RELEASE Qt6::OpenGLWidgets IMPORTED_LOCATION_RELEASE)

if (WIN32)
    get_target_property(QWindowsIntegrationPlugin_BINARY_DEBUG Qt6::QWindowsIntegrationPlugin IMPORTED_LOCATION_DEBUG)
    get_target_property(QWindowsIntegrationPlugin_BINARY_RELEASE Qt6::QWindowsIntegrationPlugin IMPORTED_LOCATION_RELEASE)
endif()


include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/3rdParty/freeimage/include)
if (WIN32)
    set(FreeImage_LIBRARY "${CMAKE_SOURCE_DIR}/3rdParty/freeimage/lib/win64/FreeImage.lib")
    set(FreeImage_BINARY  "${CMAKE_SOURCE_DIR}/3rdParty/freeimage/lib/win64/FreeImage.dll")
else()
    set(FreeImage_LIBRARY "${CMAKE_SOURCE_DIR}/3rdParty/freeimage/lib/linux64/libFreeImage.so")
    set(FreeImage_BINARY  "${CMAKE_SOURCE_DIR}/3rdParty/freeimage/lib/linux64/libFreeImage.so")
endif()

add_definitions(/DFREEIMAGE_COLORORDER=FREEIMAGE_COLORORDER_RGB)
add_definitions(/DSHIBA_VERSION_MAJOR=${SHIBA_VERSION_MAJOR})
add_definitions(/DSHIBA_VERSION_MINOR=${SHIBA_VERSION_MINOR})

set(app_sources
    ./src/AboutWidget.h
    ./src/AboutWidget.cpp
    ./src/BitmapSource.h
    ./src/BitmapSource.cpp
    ./src/CanvasWidget.h
    ./src/CanvasWidget.cpp
    ./src/Controls.h
    ./src/Controls.cpp
    ./src/DragCornerWidget.h
    ./src/DragCornerWidget.cpp
    ./src/EnumArray.h
    ./src/Exif.h
    ./src/Exif.cpp
    ./src/ExifWidget.h
    ./src/ExifWidget.cpp
    ./src/FreeImageExt.h
    ./src/FreeImageExt.cpp
    ./src/Global.h
    ./src/Global.cpp
    ./src/Histogram.h
    ./src/Histogram.cpp
    ./src/HistogramWidget.h
    ./src/HistogramWidget.cpp
    ./src/Image.h
    ./src/Image.cpp
    ./src/ImageDescription.h
    ./src/ImageDescription.cpp
    ./src/ImageInfo.h
    ./src/ImageLoader.h
    ./src/ImageLoader.cpp
    ./src/ImagePage.h
    ./src/ImagePage.cpp
    ./src/ImageProcessor.h
    ./src/ImageProcessor.cpp
    ./src/ImageSource.h
    ./src/ImageSource.cpp
    ./src/MenuWidget.h
    ./src/MenuWidget.cpp
    ./src/MultiBitmapsource.h
    ./src/MultiBitmapsource.cpp
    ./src/Pixel.h
    ./src/Pixel.cpp
    ./src/Player.h
    ./src/Player.cpp
    ./src/PluginFLO.h
    ./src/PluginFLO.cpp
    ./src/PluginSVG.h
    ./src/PluginSVG.cpp
    ./src/TextWidget.h
    ./src/TextWidget.cpp
    ./src/Tooltip.h
    ./src/Tooltip.cpp
    ./src/UniqueTick.h
    ./src/UniqueTick.cpp
    ./src/ViewerApplication.h
    ./src/ViewerApplication.cpp
    ./src/ZoomController.h
    ./src/ZoomController.cpp
    ./src/main.cpp
)

set(app_resources
    ./src/resources.rc
    ./src/assets.qrc
)

if(WIN32)
    add_executable(ShibaView WIN32 ${app_sources} ${app_resources})
else()
    add_executable(ShibaView ${app_sources} ${app_resources})
endif()

target_compile_features(ShibaView PRIVATE cxx_std_14)

target_link_libraries(ShibaView Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Svg Qt6::Charts ${FreeImage_LIBRARY})
if (UNIX)
    target_link_libraries(ShibaView dl)
endif()

set_property(TARGET ShibaView PROPERTY AUTORCC_OPTIONS "--no-compress")

install(TARGETS ShibaView DESTINATION .)
install(FILES ${FreeImage_BINARY} DESTINATION .)
install(FILES ${CMAKE_SOURCE_DIR}/LICENSE DESTINATION .)
install(FILES "${CMAKE_SOURCE_DIR}/3rdParty/freeimage/FreeImage License.txt" DESTINATION .)
install(FILES "${CMAKE_SOURCE_DIR}/fonts/Font License.txt" DESTINATION .)
install(FILES "${CMAKE_SOURCE_DIR}/icons/Icon License.txt" DESTINATION .)

get_filename_component(Qt5_BINARY_DIR ${Qt5Core_BINARY_RELEASE} DIRECTORY)
if(WIN32)
    set_target_properties(ShibaView PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    set_target_properties(ShibaView PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=${CMAKE_SOURCE_DIR}/3rdParty/freeimage/lib/win64;${Qt5_BINARY_DIR};")
endif()
install(FILES ${Qt5Core_BINARY_DEBUG} ${Qt5Gui_BINARY_DEBUG} ${Qt5Widgets_BINARY_DEBUG} ${Qt5Svg_BINARY_DEBUG} 
    ${Qt5Charts_BINARY_DEBUG} ${Qt5OpenGL_BINARY_DEBUG} ${Qt5OpenGLWidgets_BINARY_DEBUG}
    DESTINATION . 
    CONFIGURATIONS DEBUG
)
install(FILES ${Qt5Core_BINARY_RELEASE} ${Qt5Gui_BINARY_RELEASE} ${Qt5Widgets_BINARY_RELEASE} ${Qt5Svg_BINARY_RELEASE} 
    ${Qt5Charts_BINARY_RELEASE} ${Qt5OpenGL_BINARY_RELEASE} ${Qt5OpenGLWidgets_BINARY_RELEASE}
    DESTINATION . 
    CONFIGURATIONS RELEASE
)
if (WIN32)
    install(FILES ${QWindowsIntegrationPlugin_BINARY_DEBUG} DESTINATION platforms CONFIGURATIONS DEBUG)
    install(FILES ${QWindowsIntegrationPlugin_BINARY_RELEASE} DESTINATION platforms CONFIGURATIONS RELEASE)
endif()


if(WIN32)
    set(thumbnails_sources
        ./src/BitmapSource.h
        ./src/BitmapSource.cpp
        ./src/BitmapSource.h
        ./src/BitmapSource.cpp
        ./src/Exif.h
        ./src/Exif.cpp
        ./src/FreeImageExt.h
        ./src/FreeImageExt.cpp
        ./src/Global.h
        ./src/Global.cpp
        ./src/ImagePage.h
        ./src/ImagePage.cpp
        ./src/ImageSource.h
        ./src/ImageSource.cpp
        ./src/MultiBitmapsource.h
        ./src/MultiBitmapsource.cpp
        ./src/Pixel.h
        ./src/Pixel.cpp
        ./src/Player.h
        ./src/Player.cpp
        ./src/PluginFLO.h
        ./src/PluginFLO.cpp
        ./src/PluginSVG.h
        ./src/PluginSVG.cpp
        ./src/thumbnails/WindowsThumbnailProvider.h
        ./src/thumbnails/WindowsThumbnailProvider.cpp
        ./src/thumbnails/WindowsThumbnailService.cpp
        ./src/thumbnails/WindowsThumbnailService.def
    )

    add_library(ShibaThumbnail SHARED ${thumbnails_sources})

    set_target_properties(ShibaThumbnail PROPERTIES PREFIX "")

    target_compile_features(ShibaThumbnail PRIVATE cxx_std_14)

    target_link_libraries(ShibaThumbnail Shlwapi Qt6::Core Qt6::Svg ${FreeImage_LIBRARY})

    install(TARGETS ShibaThumbnail RUNTIME DESTINATION .)
endif()


set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Image viewer")
set(CPACK_PACKAGE_NAME "ShibaView")
set(CPACK_PACKAGE_VENDOR "Alexey Gruzdev")
set(CPACK_PACKAGE_VERSION_MAJOR "${SHIBA_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${SHIBA_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_COMPONENT_INCLUDE_TOPLEVEL_DIRECTORY OFF)
include (CPack)


