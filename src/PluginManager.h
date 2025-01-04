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

class PluginFlo;
class PluginSvg;

class PluginManager
{
public:
    static
    PluginManager& getInstance();

    PluginManager(const PluginManager&) = delete;
    PluginManager(PluginManager&&) = delete;

    PluginManager& operator=(const PluginManager&) = delete;
    PluginManager& operator=(PluginManager&&) = delete;


    bool initForViewer();

    bool initForThumbnails();

    FREE_IMAGE_FORMAT getFloId() const {
        return mPluginFlo.id;
    }

private:
    template <typename PluginType_>
    struct PluginCell
    {
        std::shared_ptr<PluginType_> impl{ };
        FREE_IMAGE_FORMAT id{ FIF_UNKNOWN };    // out of bounds of fi::ImageFormat
    };

    PluginManager();
    ~PluginManager();


    template <typename PluginType_, typename... Args_>
    static
    bool InitPlugin(PluginCell<PluginType_>& plugin, Args_&&... args);


    PluginCell<PluginFlo> mPluginFlo;
    PluginCell<PluginSvg> mPluginSvg;
};


#endif // PLUGINMANAGER_H

