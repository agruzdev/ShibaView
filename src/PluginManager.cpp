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

#include "PluginManager.h"

#include "PluginFLO.h"
#include "PluginSVG.h"



PluginManager& PluginManager::getInstance()
{
    static PluginManager instance;
    return instance;
}


template <typename PluginType_, typename... Args_>
bool PluginManager::InitPlugin(PluginCell<PluginType_>& plugin, Args_&&... args)
{
    PluginCell<PluginType_> res{};
    res.impl = std::make_shared<PluginType_>(std::forward<Args_>(args)...);
    res.id = static_cast<FREE_IMAGE_FORMAT>(fi::Plugin2::RegisterLocal(res.impl));
    if (res.id == FIF_UNKNOWN) {
        return false;
    }
    plugin = std::move(res);
    return true;
}


PluginManager::PluginManager() = default;


PluginManager::~PluginManager() = default;


bool PluginManager::initForViewer()
{
    bool success = true;
    success &= InitPlugin<PluginFlo>(mPluginFlo);
    success &= InitPlugin<PluginSvg>(mPluginSvg);
    return success;
}


bool PluginManager::initForThumbnails()
{
    bool success = true;
    success &= InitPlugin<PluginFlo>(mPluginFlo);
    return success;
}
