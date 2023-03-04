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

#include "ExifWidget.h"
#include <mutex>
#include <QKeyEvent>
#include "Global.h"
#include "TextWidget.h"
#include "Exif.h"

namespace
{
    struct WidgetStaticContext
    {
        ExifWidget* instance = nullptr;
        std::mutex mutex;
    };

    WidgetStaticContext gExifWidgetStaticContext{};


    QString toQString(FREE_IMAGE_MDMODEL mdmodel)
    {
        switch(mdmodel) {
        case FIMD_COMMENTS:
            return QString::fromUtf8("Comments");
        case FIMD_EXIF_MAIN:
            return QString::fromUtf8("Exif-TIFF");
        case FIMD_EXIF_EXIF:
            return QString::fromUtf8("Exif");
        case FIMD_EXIF_GPS:
            return QString::fromUtf8("GPS");
        case FIMD_EXIF_MAKERNOTE:
            return QString::fromUtf8("Exif maker");
        case FIMD_EXIF_INTEROP:
            return QString::fromUtf8("Exif interoperability");
        case FIMD_IPTC:
            return QString::fromUtf8("IPTC/NAA");
        case FIMD_XMP:
            return QString::fromUtf8("Abobe XMP");
        case FIMD_GEOTIFF:
            return QString::fromUtf8("GeoTIFF");
        case FIMD_ANIMATION:
            return QString::fromUtf8("Animation");
        default:
        case FIMD_CUSTOM:
            return QString::fromUtf8("Custom");
        }
    }

    constexpr int32_t kMinimumHeight = 300;
    constexpr int32_t kMinimumWidth  = 300;
    constexpr int32_t kMinimumPadding = 10;
}

ExifWidget& ExifWidget::getInstance()
{
    std::lock_guard<std::mutex> lock(gExifWidgetStaticContext.mutex);
    if (!gExifWidgetStaticContext.instance) {
        gExifWidgetStaticContext.instance = new ExifWidget();
    }
    return *gExifWidgetStaticContext.instance;
}


ExifWidget::ExifWidget()
    : QWidget(nullptr)
{
    setWindowFlags(Qt::WindowCloseButtonHint | Qt::MSWindowsOwnDC);
    //setAttribute(Qt::WA_DeleteOnClose);

    setWindowTitle(Global::kApplicationName + " - Exif");

    mText = new TextWidget(this, Qt::black, 11, 0.8);
    mText->setPaddings(8, 0, 4, 0);

    setFixedSize(kMinimumWidth, kMinimumHeight);
}

ExifWidget::~ExifWidget()
{
    std::lock_guard<std::mutex> lock(gExifWidgetStaticContext.mutex);
    gExifWidgetStaticContext.instance = nullptr;
}

void ExifWidget::activate()
{
    mActive = true;
}

void ExifWidget::setExif(const Exif& exif)
{
    if (mActive) {

        QVector<QString> lines;

        for (auto model : { FIMD_COMMENTS, FIMD_EXIF_MAIN, FIMD_EXIF_EXIF, FIMD_EXIF_GPS, FIMD_EXIF_MAKERNOTE,
                FIMD_EXIF_INTEROP, FIMD_IPTC, /*FIMD_XMP,*/ FIMD_GEOTIFF, /*FIMD_ANIMATION,*/ FIMD_CUSTOM })
        {
            QVector<QString> sectionLines;
            for (const auto& entry: exif.sections[model]) {
                sectionLines.push_back("  " + std::get<0>(entry) + ": " + std::get<1>(entry).toString());
            }

            if (!sectionLines.empty()) {
                lines.push_back(toQString(model) + ":");
                for (auto str: sectionLines) {
                    lines.append(std::move(str));
                }
            }
        }

        if (lines.empty()) {
            lines.push_back("N/A");
        }
 
        mText->setText(lines);

        QSize newSize = mText->size();
        newSize.setHeight(std::max(newSize.height() + kMinimumPadding, kMinimumHeight));
        newSize.setWidth(std::max(newSize.width() + kMinimumPadding, kMinimumWidth));
        setFixedSize(newSize);

        mText->update();

        update();
        show();
    }
}

void ExifWidget::setEmpty()
{
    Exif empty{};
    setExif(empty);
}

void ExifWidget::paintEvent(QPaintEvent * event)
{
    if (mActive) {
        mText->update();
    }
}

void ExifWidget::keyPressEvent(QKeyEvent *event)
{
    QWidget::keyPressEvent(event);
    if (event->key() == Qt::Key_Escape) {
        close();
    }
}

void ExifWidget::closeEvent(QCloseEvent* event)
{
    mActive = false;
    hide();
}
