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
#include <dlfcn.h>
#include <evcollect/plugin.h>
#include <evcollect/util/logging.h>

namespace evcollect {

ReturnCode SourcePlugin::pluginInit(const PluginConfig& cfg) {
  return ReturnCode::success();
}

void SourcePlugin::pluginFree() {}

ReturnCode SourcePlugin::pluginAttach(
    const PropertyList& config,
    void** userdata) {
  *userdata = nullptr;
  return ReturnCode::success();
}

void SourcePlugin::pluginDetach(void* userdata) {}

ReturnCode OutputPlugin::pluginInit(const PluginConfig& cfg) {
  return ReturnCode::success();
}

void OutputPlugin::pluginFree() {}

ReturnCode OutputPlugin::pluginAttach(
    const PropertyList& config,
    void** userdata) {
  *userdata = nullptr;
  return ReturnCode::success();
}

void OutputPlugin::pluginDetach(void* userdata) {}

ReturnCode loadPlugin(std::string plugin_path) {
  if (!StringUtil::beginsWith(plugin_path, "/") &&
      !StringUtil::beginsWith(plugin_path, "./") &&
      !StringUtil::beginsWith(plugin_path, "../")) {
    plugin_path = "./" + plugin_path;
  }

  logDebug("Loading plugin: $0", plugin_path);
  void* dl = dlopen(plugin_path.c_str(), RTLD_NOW);
  if (!dl) {
    return ReturnCode::error(
        "EIO",
        "error while loading plugin: %s: %s",
        plugin_path.c_str(),
        dlerror());
  }

  auto dl_init = dlsym(dl, "evcollect_plugin_init");
  if (!dl_init) {
    dlclose(dl);
    return ReturnCode::error(
        "EIO",
        "error while loading plugin: %s: %s [evcollect_plugin_init() not found]",
        plugin_path.c_str(),
        dlerror());
  }

  int rc = ((bool (*)(evcollect_ctx_t*)) (dl_init))(nullptr);
  if (!rc) {
    dlclose(dl);
    return ReturnCode::error(
        "EIO",
        "error while loading plugin: %s",
        plugin_path.c_str());
  }

  return ReturnCode::success();
}

} // namespace evcollect
