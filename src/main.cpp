/**
 * @file
 *
 * Copyright 2018-2023 Alexey Gruzdev
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>
#include <chrono>

#ifdef _WIN32
# include <Windows.h>
# include <WinUser.h>
#endif

#include <QApplication>
#include <QFileDialog>
#include <QSettings>
#include <QFileInfo>
#include "Global.h"
#include "ViewerApplication.h"


namespace
{
    const QString kSettingsLoadDir = "application/load_directory";
}

int main(int argc, char *argv[])
try
{
    const auto t = std::chrono::steady_clock::now();

    QApplication app{argc, argv};
    QApplication::setOrganizationName(Global::kOrganizationName);
    QApplication::setApplicationName(Global::kApplicationName);

#ifdef _MSC_VER
    std::cout << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - t).count() / 1e3 << std::endl;
#endif

#ifdef _WIN32
    do {
        auto iconHandle = LoadImage(GetModuleHandle(NULL), L"SHIBA_VIEW_ICON", IMAGE_ICON, 0, 0, 0);
        ICONINFO iconInfo{};
        if (!GetIconInfo((HICON)iconHandle, &iconInfo)) {
            break;
        }

        BITMAP bitmapInfo{};
        if (!GetObject(iconInfo.hbmColor, sizeof(bitmapInfo), &bitmapInfo)) {
            break;
        }

        BITMAP maskInfo{};
        if (!GetObject(iconInfo.hbmMask, sizeof(maskInfo), &maskInfo)) {
            break;
        }

        if (bitmapInfo.bmBitsPixel != 32 || maskInfo.bmBitsPixel != 1 || bitmapInfo.bmHeight != maskInfo.bmHeight || bitmapInfo.bmWidth != maskInfo.bmWidth) {
            break;
        }

        const LONG pixelsCount = bitmapInfo.bmHeight * bitmapInfo.bmWidth;
        const LONG colorSize   = pixelsCount * bitmapInfo.bmBitsPixel / 8;
        const LONG maskSize    = pixelsCount * maskInfo.bmBitsPixel / 8;

        auto color = std::make_unique<uint8_t[]>(colorSize);
        if (colorSize != GetBitmapBits(iconInfo.hbmColor, colorSize, color.get())) {
            break;
        }

        auto mask = std::make_unique<uint8_t[]>(maskSize);
        if (maskSize != GetBitmapBits(iconInfo.hbmMask, maskSize, mask.get())) {
            break;
        }

        QImage iconView = QImage(color.get(), bitmapInfo.bmWidth, bitmapInfo.bmHeight, bitmapInfo.bmWidthBytes, QImage::Format::Format_RGBA8888);
        QImage maskView = QImage(mask.get(),  maskInfo.bmWidth,   maskInfo.bmHeight,   maskInfo.bmWidthBytes,   QImage::Format::Format_MonoLSB);

        QPixmap iconPixmap = QPixmap::fromImage(iconView.rgbSwapped());
        iconPixmap.setMask(QBitmap::fromImage(maskView.convertedTo(QImage::Format::Format_Alpha8)));

        QApplication::setWindowIcon(QIcon(iconPixmap));

    } while(false);
#else // _WIN32
    QApplication::setWindowIcon(QIcon(":APPICON"));
#endif // _WIN32

#ifdef _MSC_VER
    std::cout << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - t).count() / 1e3 << std::endl;
#endif

    QString input;
    if (argc > 1) {
#ifdef _WIN32
        input = QApplication::arguments().at(1);
        if (input == "/register") {
            return std::system("regsvr32.exe ./ShibaThumbnail.dll");
        }
        else if (input == "/unregister") {
            return std::system("regsvr32.exe /u ./ShibaThumbnail.dll");
        }
#else // _WIN32
        std::cout << "Thumbnail service is available only on Windows" << std::endl;
        std::exit(1);
#endif // _WIN32
    }
    else {
        QSettings settings;
        input = QFileDialog::getOpenFileName(nullptr, "Open File", settings.value(kSettingsLoadDir, "/").toString(), Global::getSupportedExtensionsFilterString() + ";;All files (*.*)");
        if (!input.isEmpty()) {
            settings.setValue(kSettingsLoadDir, QFileInfo(input).dir().absolutePath());
        }
    }

    if (input.isEmpty()) {
        return 0;
    }

    ViewerApplication viewer(t);
    viewer.open(input);

#ifdef _MSC_VER
    std::cout << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - t).count() / 1e3 << std::endl;
#endif
    return QApplication::exec();
}
catch(...){
    return -1;
}
