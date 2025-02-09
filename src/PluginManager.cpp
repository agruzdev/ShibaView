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

#if SHIBAVIEW_APPLICATION
# include <QMessageBox>
#endif

#include "Global.h"
#include "PluginFLO.h"
#include "PluginSVG.h"
#include "PluginSvgCairo.h"


namespace
{

    constexpr
    bool checkOneBitIsSet(uint32_t v) {
        return (v > 0) && ((v & (v - 1)) == 0);
    }

} // namespace


PluginManager& PluginManager::getInstance()
{
    static PluginManager instance;
    return instance;
}


template <typename PluginType_, typename... Args_>
bool PluginManager::InitOrUpdatePlugin(PluginCell& plugin, Args_&&... args)
{
    PluginCell res{};
    res.impl = std::make_shared<PluginType_>(std::forward<Args_>(args)...);
    if (plugin.id != fi::ImageFormat::eUnknown) {
        if (fi::Plugin2::ResetLocalPlugin(plugin.id, res.impl, /*force=*/true)) {
            res.id = plugin.id;
        }
    }
    else {
        res.id = fi::Plugin2::RegisterLocal(res.impl);
    }
    if (res.id == fi::ImageFormat::eUnknown) {
        return false;
    }
    plugin = std::move(res);
    return true;
}


void PluginManager::UnloadPlugin(PluginCell& plugin)
{
    if (plugin.id != fi::ImageFormat::eUnknown) {
        fi::Plugin2::ResetLocalPlugin(static_cast<fi::ImageFormat>(plugin.id), nullptr);
        plugin.impl = nullptr;
        plugin.id = fi::ImageFormat::eUnknown;
    }
}


PluginManager::PluginManager() = default;


PluginManager::~PluginManager() = default;


bool PluginManager::init(PluginUsage usage)
{
    if (mInitialized) {
        throw std::runtime_error("PluginManager[init]: Already initialized.");
    }
    if (!checkOneBitIsSet(static_cast<std::underlying_type_t<PluginUsage>>(usage))) {
        throw std::runtime_error("PluginManager[init]: Invalid usage.");
    }
    mTargetUsage = usage;
    mSettings = Settings::getSettings(Settings::Group::ePlugins);
    const bool success = reload();
    mInitialized = true;
    return success;
}


bool PluginManager::reload()
{
    bool success = true;
    success &= setupPluginFlo();
    success &= setupPluginSvg();
    return success;
}


bool PluginManager::setupPluginFlo()
try
{
    mPluginFlo.usageMask = static_cast<PluginUsage>(mSettings->value(Settings::kPluginFloUsage, Settings::kPluginFloUsageDefault).toUInt());
    if (testFlag(mPluginFlo.usageMask, mTargetUsage)) {
        return InitOrUpdatePlugin<PluginFlo>(mPluginFlo);
    }
    UnloadPlugin(mPluginFlo);
    return true;
}
catch (std::exception& err) {
#if SHIBAVIEW_APPLICATION
    QMessageBox::warning(nullptr, Global::kApplicationName + " - Error", "Failed to load plugin 'FLO'. Reason:\n" + QString::fromUtf8(err.what()));
#endif
    return false;
}


bool PluginManager::setupPluginSvg()
try
{
    mPluginSvg.usageMask = static_cast<PluginUsage>(mSettings->value(Settings::kPluginSvgUsage, Settings::kPluginSvgUsageDefault).toUInt());
    if (testFlag(mPluginSvg.usageMask, mTargetUsage)) {
        const QString libcairo = mSettings->value(Settings::kPluginSvgLibcairo, QString{}).toString();
        const QString librsvg  = mSettings->value(Settings::kPluginSvgLibrsvg,  QString{}).toString();
        if (!libcairo.isEmpty() && !librsvg.isEmpty()) {
            return InitOrUpdatePlugin<PluginSvgCairo>(mPluginSvg, libcairo, librsvg);
        }
        else {
            return InitOrUpdatePlugin<PluginSvg>(mPluginSvg);
        }
    }
    UnloadPlugin(mPluginSvg);
    return true;
}
catch (std::exception& err) {
#if SHIBAVIEW_APPLICATION
    QMessageBox::warning(nullptr, Global::kApplicationName + " - Error", "Failed to load plugin 'SVG'. Reason:\n" + QString::fromUtf8(err.what()));
#endif
    return false;
}
