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

#ifndef GLOBAL_H
#define GLOBAL_H

#include <memory>
#include <QString>
#include <QStringList>
#include <QSettings>

#define UTF8_DEGREE "\xC2\xB0"
#define UTF8_GAMMA  "\xCE\xB3"

class Global
{
public:
    static constexpr uint32_t kVersionMajor = SHIBA_VERSION_MAJOR;

    static constexpr uint32_t kVersionMinor = SHIBA_VERSION_MINOR;

    static const QString kApplicationName;

    static const QString kOrganizationName;

    static const QString kDefaultFont;

    static const QStringList& getSupportedExtensions() noexcept;

    static QStringList getSupportedExtensionFilters();

    static QString getSupportedExtensionsFilterString();



    enum class SettingsGroup
    {
        eGlobal,
        eControls
    };

    // not null
    static std::unique_ptr<QSettings> getSettings(SettingsGroup group);

    // [Global]
    static const QString kParamBackgroundKey;
    static const QString kParamBackgroundDefault;

private:
    Global() = delete;
};

#endif // GLOBAL_H
