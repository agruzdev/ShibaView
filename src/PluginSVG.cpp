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

#include "PluginFLO.h"

#include <QDebug>
#include <QSvgRenderer>
#include <QImage>
#include <QPainter>
#include <QRect>

namespace
{
    std::unique_ptr<QByteArray> loadXmlBuffer(FreeImageIO* io, fi_handle handle)
    {
        const auto posStart = io->tell_proc(handle);
        io->seek_proc(handle, 0, SEEK_END);
        const auto posEnd = io->tell_proc(handle);
        io->seek_proc(handle, posStart, SEEK_SET);

        if (posEnd <= posStart) {
            return nullptr;
        }

        const auto xmlSize = static_cast<uint32_t>(posEnd - posStart);
        auto xmlBuffer = std::make_unique<QByteArray>(xmlSize, Qt::Initialization::Uninitialized);
        if (xmlSize != io->read_proc(xmlBuffer->data(), sizeof(char), xmlSize, handle)) {
            return nullptr;
        }

        return xmlBuffer;
    }
}

void initPluginSVG(Plugin *plugin, int format_id)
{
    (void)format_id;
    assert(FIEF_SVG == format_id);

    plugin->format_proc = []() -> const char* {
        return "SVG";
    };

    plugin->description_proc = []() -> const char* {
        return "Scalable Vector Graphics";
    };

    plugin->extension_proc = []() -> const char* {
        return "svg";
    };

    plugin->load_proc = [](FreeImageIO* io, fi_handle handle, int /*page*/, int /*flags*/, void* /*data*/) -> FIBITMAP* {
        try {
            const auto xmlBuffer = loadXmlBuffer(io, handle);
            if (!xmlBuffer) {
                qDebug() << "Failed to read xml buffer";
                return nullptr;
            }

            QXmlStreamReader xmlReader(*xmlBuffer);
            if (xmlReader.hasError()) {
                qDebug() << xmlReader.error();
                return nullptr;
            }

            QSvgRenderer svgRenderer(&xmlReader);
            if (!svgRenderer.isValid()) {
                qDebug() << "QSvgRenderer is not valid";
                return nullptr;
            }

            QSize svgSize = svgRenderer.defaultSize();
            if (svgSize.isEmpty()) {
                svgSize = QSize(1024, 1024);
            }

            std::unique_ptr<FIBITMAP, decltype(&::FreeImage_Unload)> bmp(FreeImage_Allocate(svgSize.width(), svgSize.height(), 32), &::FreeImage_Unload);
            QImage rgbaView(FreeImage_GetBits(bmp.get()), FreeImage_GetWidth(bmp.get()), FreeImage_GetHeight(bmp.get()), FreeImage_GetPitch(bmp.get()), QImage::Format::Format_RGBA8888);

            QPainter painter(&rgbaView);

            const auto center = QRectF(0, 0, svgSize.width(), svgSize.height()).center();
            const auto trans1 = QTransform::fromTranslate(-center.x(), -center.y());
            const auto scale  = QTransform::fromScale(1.0, -1.0);
            const auto trans2 = QTransform::fromTranslate(center.x(), center.y());
            painter.setTransform(trans1 * scale * trans2);

            svgRenderer.render(&painter);

            return bmp.release();
        }
        catch (std::exception& err) {
            qDebug() << "Failed to render svg";
        }
        catch (...) {
        }
        return nullptr;
    };

    plugin->save_proc = [](FreeImageIO* /*io*/, FIBITMAP* /*dib*/, fi_handle /*handle*/, int /*page*/, int /*flags*/, void* /*data*/) -> BOOL {
        return FALSE;
    };

    plugin->validate_proc = [](FreeImageIO* io, fi_handle handle) -> BOOL {
        // ToDo (a.gruzdev): Is it possible to not read the entire file?
#if 0
        const auto xmlBuffer = loadXmlBuffer(io, handle);
        if (!xmlBuffer) {
            qDebug() << "Failed to read xml buffer";
            return FALSE;
        }

        QXmlStreamReader xmlReader(*xmlBuffer);
        if (xmlReader.hasError()) {
            qDebug() << xmlReader.error();
            return FALSE;
        }

        int maxTagsToRead = 8;
        while(!xmlReader.atEnd()) {
            xmlReader.readNext();
            if (xmlReader.name() == QString("svg")) {
                return TRUE;
            }
            if (!(maxTagsToRead--)) {
                break;
            }
        }
#endif
        return FALSE;
    };

}
