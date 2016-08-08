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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
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

DynamicSourcePlugin::DynamicSourcePlugin(
    PluginContext* ctx,
    evcollect_plugin_getnextevent_fn getnextevent_fn,
    evcollect_plugin_hasnextevent_fn hasnextevent_fn,
    evcollect_plugin_attach_fn attach_fn,
    evcollect_plugin_detach_fn detach_fn,
    evcollect_plugin_init_fn init_fn,
    evcollect_plugin_free_fn free_fn) :
    ctx_(ctx),
    getnextevent_fn_(getnextevent_fn),
    hasnextevent_fn_(hasnextevent_fn),
    attach_fn_(attach_fn),
    detach_fn_(detach_fn),
    init_fn_(init_fn),
    free_fn_(free_fn) {}

ReturnCode DynamicSourcePlugin::pluginInit(const PluginConfig& cfg) {
  if (!init_fn_ || init_fn_(ctx_)) {
    return ReturnCode::success();
  } else {
    return ReturnCode::error(
        "EPLUGIN",
        "pluginInit failed: %s",
        ctx_->error.c_str());
  }
}

void DynamicSourcePlugin::pluginFree() {
  if (free_fn_) {
    free_fn_(ctx_);
  }
}

ReturnCode DynamicSourcePlugin::pluginAttach(
    const PropertyList& config,
    void** userdata) {
  if (!attach_fn_ || attach_fn_(ctx_, &config, userdata)) {
    return ReturnCode::success();
  } else {
    return ReturnCode::error(
        "EPLUGIN",
        "pluginAttach failed: %s",
        ctx_->error.c_str());
  }
}

void DynamicSourcePlugin::pluginDetach(void* userdata) {
  if (detach_fn_) {
    detach_fn_(ctx_, userdata);
  }
}

ReturnCode DynamicSourcePlugin::pluginGetNextEvent(
    void* userdata,
    std::string* data) {
  EventData evdata;

  if (getnextevent_fn_(ctx_, userdata, &evdata)) {
    *data = evdata.event_data;
    return ReturnCode::success();
  } else {
    return ReturnCode::error(
        "EPLUGIN",
        "pluginGetNextEvent failed: %s",
        ctx_->error.c_str());
  }
}

bool DynamicSourcePlugin::pluginHasPendingEvent(void* userdata) {
  if (!hasnextevent_fn_) {
    return false;
  } else {
    return hasnextevent_fn_(ctx_, userdata);
  }
}

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

DynamicOutputPlugin::DynamicOutputPlugin(
    PluginContext* ctx,
    evcollect_plugin_emitevent_fn emitevent_fn,
    evcollect_plugin_attach_fn attach_fn,
    evcollect_plugin_detach_fn detach_fn,
    evcollect_plugin_init_fn init_fn,
    evcollect_plugin_free_fn free_fn) :
    ctx_(ctx),
    emitevent_fn_(emitevent_fn),
    attach_fn_(attach_fn),
    detach_fn_(detach_fn),
    init_fn_(init_fn),
    free_fn_(free_fn) {}

ReturnCode DynamicOutputPlugin::pluginInit(const PluginConfig& cfg) {
  if (!init_fn_ || init_fn_(ctx_)) {
    return ReturnCode::success();
  } else {
    return ReturnCode::error(
        "EPLUGIN",
        "pluginInit failed: %s",
        ctx_->error.c_str());
  }
}

void DynamicOutputPlugin::pluginFree() {
  if (free_fn_) {
    free_fn_(ctx_);
  }
}

ReturnCode DynamicOutputPlugin::pluginAttach(
    const PropertyList& config,
    void** userdata) {
  if (!attach_fn_ || attach_fn_(ctx_, &config, userdata)) {
    return ReturnCode::success();
  } else {
    return ReturnCode::error(
        "EPLUGIN",
        "pluginAttach failed: %s",
        ctx_->error.c_str());
  }
}

void DynamicOutputPlugin::pluginDetach(void* userdata) {
  if (detach_fn_) {
    detach_fn_(ctx_, userdata);
  }
}

ReturnCode DynamicOutputPlugin::pluginEmitEvent(
    void* userdata,
    const EventData& event) {
  if (emitevent_fn_(ctx_, userdata, &event)) {
    return ReturnCode::success();
  } else {
    return ReturnCode::error(
        "EPLUGIN",
        "pluginEmitEvent failed: %s",
        ctx_->error.c_str());
  }
}

ReturnCode loadPlugin(PluginContext* plugin_ctx, std::string plugin_path) {
  logInfo("Loading plugin: $0", plugin_path);
  void* dl = dlopen(plugin_path.c_str(), RTLD_NOW);
  if (!dl) {
    return ReturnCode::error(
        "EIO",
        "error while loading plugin: %s: %s",
        plugin_path.c_str(),
        dlerror());
  }

  auto dl_init = dlsym(dl, "__evcollect_plugin_init");
  if (!dl_init) {
    dlclose(dl);
    return ReturnCode::error(
        "EIO",
        "error while loading plugin: %s: %s [evcollect_plugin_init() not found]",
        plugin_path.c_str(),
        dlerror());
  }

  int rc = ((bool (*)(evcollect_ctx_t*)) (dl_init))(plugin_ctx);
  if (!rc) {
    dlclose(dl);
    return ReturnCode::error(
        "EIO",
        "error while loading plugin: %s: %s",
        plugin_path.c_str(),
        plugin_ctx->error.c_str());
  }

  return ReturnCode::success();
}

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
    path_candidates.emplace_back(config_->plugin_dir + "/" + plugin_name);
    path_candidates.emplace_back(
        config_->plugin_dir + "/plugin_" + plugin_name + ".so");
  }

  for (const auto& path : path_candidates) {
    struct stat s;
    if (stat(path.c_str(), &s) != 0) {
      continue;
    }

    return evcollect::loadPlugin(ctx, path);
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

void evcollect_log(
    evcollect_loglevel level,
    const char* msg) {
  switch (level) {
    case EVCOLLECT_LOG_FATAL:
      logFatal(msg);
      break;
    case EVCOLLECT_LOG_EMERGENCY:
      logEmergency(msg);
      break;
    case EVCOLLECT_LOG_ALERT:
      logAlert(msg);
      break;
    case EVCOLLECT_LOG_CRITICAL:
      logCritical(msg);
      break;
    case EVCOLLECT_LOG_ERROR:
      logError(msg);
      break;
    case EVCOLLECT_LOG_WARNING:
      logWarning(msg);
      break;
    case EVCOLLECT_LOG_NOTICE:
      logNotice(msg);
      break;
    case EVCOLLECT_LOG_INFO:
      logInfo(msg);
      break;
    case EVCOLLECT_LOG_DEBUG:
      logDebug(msg);
      break;
    case EVCOLLECT_LOG_TRACE:
      logTrace(msg);
      break;
  }
}

void evcollect_seterror(evcollect_ctx_t* ctx, const char* error) {
  auto ctx_ = static_cast<evcollect::PluginContext*>(ctx);
  ctx_->error = std::string(error);
}

bool evcollect_plugin_getcfg(
    const evcollect_plugin_cfg_t* cfg,
    const char* key,
    const char** value) {
  auto cfg_ = static_cast<const evcollect::PropertyList*>(cfg);
  return cfg_->get(key, value);
}

bool evcollect_plugin_getcfgv(
    const evcollect_plugin_cfg_t* cfg,
    const char* key,
    size_t i,
    size_t j,
    const char** value) {
  auto cfg_ = static_cast<const evcollect::PropertyList*>(cfg);
  return cfg_->getv(key, i, j, value);
}

void evcollect_event_getname(
    const evcollect_event_t* ev,
    const char** data,
    size_t* size) {
  auto ev_ = static_cast<const evcollect::EventData*>(ev);
  *data = ev_->event_name.data();
  *size = ev_->event_name.size();
}

void evcollect_event_setname(
    evcollect_event_t* ev,
    const char* data,
    size_t size) {
  auto ev_ = static_cast<evcollect::EventData*>(ev);
  ev_->event_name = std::string(data, size);
}

void evcollect_event_getdata(
    const evcollect_event_t* ev,
    const char** data,
    size_t* size) {
  auto ev_ = static_cast<const evcollect::EventData*>(ev);
  *data = ev_->event_data.data();
  *size = ev_->event_data.size();
}

void evcollect_event_setdata(
    evcollect_event_t* ev,
    const char* data,
    size_t size) {
  auto ev_ = static_cast<evcollect::EventData*>(ev);
  ev_->event_data = std::string(data, size);
}

void evcollect_source_plugin_register(
    evcollect_ctx_t* ctx,
    const char* plugin_name,
    evcollect_plugin_getnextevent_fn getnextevent_fn,
    evcollect_plugin_hasnextevent_fn hasnextevent_fn /* = nullptr */,
    evcollect_plugin_attach_fn attach_fn /* = nullptr */,
    evcollect_plugin_detach_fn detach_fn /* = nullptr */,
    evcollect_plugin_init_fn init_fn /* = nullptr */,
    evcollect_plugin_free_fn free_fn /* = nullptr */) {
  auto ctx_ = static_cast<evcollect::PluginContext*>(ctx);
  ctx_->plugin_map->registerSourcePlugin(
      plugin_name,
      std::unique_ptr<evcollect::SourcePlugin>(
          new evcollect::DynamicSourcePlugin(
              ctx_,
              getnextevent_fn,
              hasnextevent_fn,
              attach_fn,
              detach_fn,
              init_fn,
              free_fn)));
}

void evcollect_output_plugin_register(
    evcollect_ctx_t* ctx,
    const char* plugin_name,
    evcollect_plugin_emitevent_fn emitevent_fn,
    evcollect_plugin_attach_fn attach_fn /* = nullptr */,
    evcollect_plugin_detach_fn detach_fn /* = nullptr */,
    evcollect_plugin_init_fn init_fn /* = nullptr */,
    evcollect_plugin_free_fn free_fn /* = nullptr */) {
  auto ctx_ = static_cast<evcollect::PluginContext*>(ctx);
  ctx_->plugin_map->registerOutputPlugin(
      plugin_name,
      std::unique_ptr<evcollect::OutputPlugin>(
          new evcollect::DynamicOutputPlugin(
              ctx_,
              emitevent_fn,
              attach_fn,
              detach_fn,
              init_fn,
              free_fn)));
}

