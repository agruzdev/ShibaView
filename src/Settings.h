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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <memory>
#include <QString>
#include <QSettings>


class Settings
{
public:
    enum class Group
    {
        eGlobal,
        eControls
    };

    // not null
    static std::unique_ptr<QSettings> getSettings(Group group);

    // [Global]
    static const QString kParamBackgroundKey;
    static const QString kParamBackgroundDefault;
    static const QString kParamTextColorKey;
    static const QString kParamTextColorDefault;
    static const QString kParamShowCloseButtonKey;
    static const QString kParamShowCloseButtonDefault;

private:
    Settings() = delete;
};

#endif // SETTINGS_H
