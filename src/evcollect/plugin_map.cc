/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include <unistd.h>
#include <evcollect/evcollect.h>
#include <evcollect/plugin_map.h>
#include <evcollect/plugin.h>
#include <evcollect/config.h>
#include <evcollect/util/stringutil.h>

namespace evcollect {

PluginMap::PluginMap(const ProcessConfig* config) : config_(config) {}

PluginMap::~PluginMap() {
  for (auto& plugin : source_plugins_) {
    if (!plugin.second.plugin_initialized) {
      continue;
    }

    plugin.second.plugin->pluginFree();
  }
}

ReturnCode PluginMap::loadPlugin(
    const std::string& plugin_name,
    PluginContext* ctx) const {
  std::vector<std::string> path_candidates;
  path_candidates.emplace_back(plugin_name);

  if (plugin_name.find("/") == std::string::npos) {
    path_candidates.emplace_back(config_->plugin_dir + plugin_name);
    path_candidates.emplace_back(
        config_->plugin_dir + "plugin_" + plugin_name + ".so");
  }

  for (const auto& path : path_candidates) {
    if (access(path.c_str(), F_OK) != 0) {
      continue;
    }

    return evcollect::loadPlugin(ctx, plugin_name);
  }

  return ReturnCode::error(
      "EPLUGIN",
      StringUtil::format(
          "plugin not found -- tried $0",
          StringUtil::join(path_candidates, ", ")));
}

void PluginMap::registerSourcePlugin(
    const std::string& plugin_name,
    std::unique_ptr<SourcePlugin> plugin) {
  SourcePluginBinding plugin_binding;
  plugin_binding.plugin = std::move(plugin);
  plugin_binding.plugin_initialized = false;
  source_plugins_.emplace(plugin_name, std::move(plugin_binding));
}

ReturnCode PluginMap::getSourcePlugin(
    const std::string& plugin_name,
    SourcePlugin** plugin) const {
  auto iter = source_plugins_.find(plugin_name);
  if (iter == source_plugins_.end()) {
    return ReturnCode::error(
        "PLUGIN_NOT_FOUND",
        "plugin not found: %s",
        plugin_name.c_str());
  }

  auto& plugin_iter = iter->second;
  if (!plugin_iter.plugin_initialized) {
    PluginConfig pc;
    pc.spool_dir = config_->spool_dir;

    auto rc = plugin_iter.plugin->pluginInit(pc);
    if (!rc.isSuccess()) {
      return rc;
    }

    plugin_iter.plugin_initialized = true;
  }

  *plugin = plugin_iter.plugin.get();
  return ReturnCode::success();
}

void PluginMap::registerOutputPlugin(
    const std::string& plugin_name,
    std::unique_ptr<OutputPlugin> plugin) {
  OutputPluginBinding plugin_binding;
  plugin_binding.plugin = std::move(plugin);
  plugin_binding.plugin_initialized = false;
  output_plugins_.emplace(plugin_name, std::move(plugin_binding));
}

ReturnCode PluginMap::getOutputPlugin(
    const std::string& plugin_name,
    OutputPlugin** plugin) const {
  auto iter = output_plugins_.find(plugin_name);
  if (iter == output_plugins_.end()) {
    return ReturnCode::error(
        "PLUGIN_NOT_FOUND",
        "output plugin not found: %s",
        plugin_name.c_str());
  }

  auto& plugin_iter = iter->second;
  if (!plugin_iter.plugin_initialized) {
    PluginConfig pc;
    pc.spool_dir = config_->spool_dir;

    auto rc = plugin_iter.plugin->pluginInit(pc);
    if (!rc.isSuccess()) {
      return rc;
    }

    plugin_iter.plugin_initialized = true;
  }

  *plugin = plugin_iter.plugin.get();
  return ReturnCode::success();
}

} // namespace evcollect
