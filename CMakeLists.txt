#
# Copyright 2018-2021 Alexey Gruzdev
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

cmake_minimum_required(VERSION 3.10)

project(ShibaView)

set(SHIBA_VERSION_MAJOR 1)
set(SHIBA_VERSION_MINOR 0)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


find_package(Qt6 COMPONENTS Core Gui Widgets QWindowsIntegrationPlugin REQUIRED)

get_target_property(Qt5Core_BINARY_DEBUG Qt6::Core IMPORTED_LOCATION_DEBUG)
get_target_property(Qt5Core_BINARY_RELEASE Qt6::Core IMPORTED_LOCATION_RELEASE)

get_target_property(Qt5Gui_BINARY_DEBUG Qt6::Gui IMPORTED_LOCATION_DEBUG)
get_target_property(Qt5Gui_BINARY_RELEASE Qt6::Gui IMPORTED_LOCATION_RELEASE)

get_target_property(Qt5Widgets_BINARY_DEBUG Qt6::Widgets IMPORTED_LOCATION_DEBUG)
get_target_property(Qt5Widgets_BINARY_RELEASE Qt6::Widgets IMPORTED_LOCATION_RELEASE)

get_target_property(QWindowsIntegrationPlugin_BINARY_DEBUG Qt6::QWindowsIntegrationPlugin IMPORTED_LOCATION_DEBUG)
get_target_property(QWindowsIntegrationPlugin_BINARY_RELEASE Qt6::QWindowsIntegrationPlugin IMPORTED_LOCATION_RELEASE)


include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/3rdParty/freeimage/include)
set(FreeImage_LIBRARY "${CMAKE_SOURCE_DIR}/3rdParty/freeimage/lib/FreeImage.lib")
set(FreeImage_BINARY  "${CMAKE_SOURCE_DIR}/3rdParty/freeimage/lib/FreeImage.dll")

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
    ./src/EnumArray.h
    ./src/Exif.h
    ./src/Exif.cpp
    ./src/ExifWidget.h
    ./src/ExifWidget.cpp
    ./src/FreeImageExt.h
    ./src/FreeImageExt.cpp
    ./src/Global.h
    ./src/Global.cpp
    ./src/Image.h
    ./src/Image.cpp
    ./src/ImageDescription.h
    ./src/ImageDescription.cpp
    ./src/ImageInfo.h
    ./src/ImageLoader.h
    ./src/ImageLoader.cpp
    ./src/ImagePage.h
    ./src/ImagePage.cpp
    ./src/ImagePageFLO.h
    ./src/ImagePageFLO.cpp
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

add_executable(ShibaView WIN32 ${app_sources} ${app_resources})

target_compile_features(ShibaView PRIVATE cxx_std_14)

target_link_libraries(ShibaView Qt6::Core Qt6::Gui Qt6::Widgets ${FreeImage_LIBRARY})

install(TARGETS ShibaView DESTINATION .)
install(FILES ${FreeImage_BINARY} DESTINATION .)
install(FILES ${CMAKE_SOURCE_DIR}/LICENSE DESTINATION .)
install(FILES "${CMAKE_SOURCE_DIR}/3rdParty/freeimage/FreeImage License.txt" DESTINATION .)
install(FILES "${CMAKE_SOURCE_DIR}/fonts/Font License.txt" DESTINATION .)
install(FILES "${CMAKE_SOURCE_DIR}/icons/Icon License.txt" DESTINATION .)

get_filename_component(Qt5_BINARY_DIR ${Qt5Core_BINARY_RELEASE} DIRECTORY)
set_target_properties(ShibaView PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set_target_properties(ShibaView PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=${CMAKE_SOURCE_DIR}/3rdParty/freeimage/lib;${Qt5_BINARY_DIR};")
install(FILES ${Qt5Core_BINARY_DEBUG} ${Qt5Gui_BINARY_DEBUG} ${Qt5Widgets_BINARY_DEBUG} DESTINATION . CONFIGURATIONS DEBUG)
install(FILES ${Qt5Core_BINARY_RELEASE} ${Qt5Gui_BINARY_RELEASE} ${Qt5Widgets_BINARY_RELEASE} DESTINATION . CONFIGURATIONS RELEASE)
install(FILES ${QWindowsIntegrationPlugin_BINARY_DEBUG} DESTINATION platforms CONFIGURATIONS DEBUG)
install(FILES ${QWindowsIntegrationPlugin_BINARY_RELEASE} DESTINATION platforms CONFIGURATIONS RELEASE)



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
        ./src/ImagePageFLO.h
        ./src/ImagePageFLO.cpp
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
        ./src/thumbnails/WindowsThumbnailProvider.h
        ./src/thumbnails/WindowsThumbnailProvider.cpp
        ./src/thumbnails/WindowsThumbnailService.cpp
        ./src/thumbnails/WindowsThumbnailService.def
    )

    add_library(ShibaThumbnail SHARED ${thumbnails_sources})

    set_target_properties(ShibaThumbnail PROPERTIES PREFIX "")

    target_compile_features(ShibaThumbnail PRIVATE cxx_std_14)

    target_link_libraries(ShibaThumbnail Shlwapi Qt6::Core ${FreeImage_LIBRARY})

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


