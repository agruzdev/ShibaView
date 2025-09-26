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

#include "Settings.h"
#include "PluginManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>

namespace
{
    const QString kSettingsFileName = "Settings.ini";

    QString ToString(Settings::Group group)
    {
        switch (group) {
        case Settings::Group::eControls:
            return "Controls";
        case Settings::Group::eGlobal:
            return "Global";
        case Settings::Group::ePlugins:
            return "Plugins";
        default:
            assert(false);
            return QString();
        }
    }
}

std::unique_ptr<QSettings> Settings::getSettings(Settings::Group group)
{
    const QString absSettingsPath = QDir(QCoreApplication::applicationDirPath()).filePath(kSettingsFileName);
    auto settings = std::make_unique<QSettings>(absSettingsPath, QSettings::Format::IniFormat);
    settings->beginGroup(ToString(group));
    return settings;
}

// [Global]
const QString Settings::kParamBackgroundKey = "Background";
const QString Settings::kParamBackgroundDefault = "#2B2B2B";
const QString Settings::kParamTextColorKey = "TextColor";
const QString Settings::kParamTextColorDefault = "#FFFFFF";
const QString Settings::kParamShowCloseButtonKey = "ShowCloseButton";
const QString Settings::kParamShowCloseButtonDefault = "0";
const QString Settings::kParamInvertZoom = "InvertZoom";
const QString Settings::kParamInvertZoomDefault = "0";

// [Plugins]
const QString  Settings::kPluginFloUsage  = "Flo";
const uint32_t Settings::kPluginFloUsageDefault = static_cast<uint32_t>(PluginUsage::eViewer | PluginUsage::eThumbnails);
const QString  Settings::kPluginSvgUsage  = "Svg";
const uint32_t Settings::kPluginSvgUsageDefault = static_cast<uint32_t>(PluginUsage::eViewer);
const QString  Settings::kPluginSvgLibcairo = "SvgLibcairo";
const QString  Settings::kPluginSvgLibrsvg  = "SvgLibrsvg";

