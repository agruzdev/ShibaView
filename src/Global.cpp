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

#include "Global.h"

#include <QApplication>
#include <QDir>
#include <QFile>

namespace
{
    const QString kSettingsFileName = "Settings.ini";

    const QStringList kSupportedExtensions = {
        ".png", ".pns",
        ".jpg", ".jpeg", ".jpe",
        ".jpf", ".jpx", ".jp2", ".j2c", ".j2k", ".jpc",
        ".tga", ".targa",
        ".tif", ".tiff",
        ".exr",
        ".bmp",
        ".gif",
        ".pbm", ".pgm", ".ppm", ".pnm", ".pfm", ".pam",
        ".hdr",
        ".webp",
        ".dds",
        ".iff", ".tdi",
        ".pcx",
        ".psd",
        ".svg",
        ".flo"
    };

    QStringList cvtExtensionsToFilters(const QStringList& exts)
    {
        QStringList filters;
        filters.reserve(exts.size());
        for (const auto& ext : exts) {
            filters.push_back("*" + ext);
        }
        return filters;
    }

    QString ToString(Global::SettingsGroup group)
    {
        switch (group) {
        case Global::SettingsGroup::eControls:
            return "Controls";
        case Global::SettingsGroup::eGlobal:
            return "Global";
        default:
            assert(false);
            return QString();
        }
    }
}

QStringList Global::getSupportedExtensionFilters()
{
    return cvtExtensionsToFilters(getSupportedExtensions());
}

QString Global::getSupportedExtensionsFilterString()
{
    return "Images (" + getSupportedExtensionFilters().join(" ") + ")";
}

const QString Global::kApplicationName = "ShibaView";

const QString Global::kOrganizationName = "Alexey Gruzdev";

const QString Global::kDefaultFont = ":/fonts/DejaVuSansCondensed.ttf";

const QStringList& Global::getSupportedExtensions() noexcept
{
    return kSupportedExtensions;
}

std::unique_ptr<QSettings> Global::getSettings(SettingsGroup group)
{
    const QString absSettingsPath = QDir(QApplication::applicationDirPath()).filePath(kSettingsFileName);
    auto settings = std::make_unique<QSettings>(absSettingsPath, QSettings::Format::IniFormat);
    settings->beginGroup(ToString(group));
    return settings;
}

// [Global]
const QString Global::kParamBackgroundKey = "Background";
const QString Global::kParamBackgroundDefault = "#2B2B2B";
