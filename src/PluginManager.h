/**
 * @file
 *
 * Copyright 2018-2024 Alexey Gruzdev
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

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <memory>
#include "FreeImage.hpp"
#include "Settings.h"


enum class PluginUsage
    : uint32_t
{
    eNone       = 0,
    eViewer     = 1 << 0,
    eThumbnails = 1 << 1
};

inline
PluginUsage operator| (PluginUsage lhs, PluginUsage rhs)
{
    return static_cast<PluginUsage>(static_cast<std::underlying_type_t<PluginUsage>>(lhs) | static_cast<std::underlying_type_t<PluginUsage>>(rhs));
}

inline
PluginUsage operator& (PluginUsage lhs, PluginUsage rhs)
{
    return static_cast<PluginUsage>(static_cast<std::underlying_type_t<PluginUsage>>(lhs) & static_cast<std::underlying_type_t<PluginUsage>>(rhs));
}


class PluginManager
{
public:
    static
    PluginManager& getInstance();

    PluginManager(const PluginManager&) = delete;
    PluginManager(PluginManager&&) = delete;

    PluginManager& operator=(const PluginManager&) = delete;
    PluginManager& operator=(PluginManager&&) = delete;


    bool init(PluginUsage usage = PluginUsage::eThumbnails);

    bool reload();


    FREE_IMAGE_FORMAT getFloId() const {
        return static_cast<FREE_IMAGE_FORMAT>(mPluginFlo.id);
    }


private:
    struct PluginCell
    {
        std::shared_ptr<fi::Plugin2> impl{ };
        fi::ImageFormat id{ fi::ImageFormat::eUnknown };
        PluginUsage usageMask{ PluginUsage::eNone };
    };

    PluginManager();
    ~PluginManager();


    template <typename PluginType_, typename... Args_>
    static
    bool InitOrUpdatePlugin(PluginCell& plugin, Args_&&... args);

    void UnloadPlugin(PluginCell& plugin);


    bool setupPluginFlo();

    bool setupPluginSvg();


    bool mInitialized{ false };
    PluginUsage mTargetUsage{ PluginUsage::eNone };
    std::unique_ptr<QSettings> mSettings;
    PluginCell mPluginFlo;
    PluginCell mPluginSvg;
};


#endif // PLUGINMANAGER_H

