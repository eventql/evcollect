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
#pragma once
#include <string>
#include <evcollect/evcollect.h>
#include <evcollect/config.h>
#include <evcollect/util/return_code.h>

namespace evcollect {
class PluginMap;

struct PluginConfig {
  std::string spool_dir;
};

struct PluginContext {
  std::string error;
  PluginMap* plugin_map;
};

class SourcePlugin {
public:

  virtual ~SourcePlugin() = default;

  /**
   * Called when the daemon is started
   */
  virtual ReturnCode pluginInit(const PluginConfig& cfg);

  /**
   * Called when the daemon is stopped
   */
  virtual void pluginFree();

  /**
   * Called for each event definition the plugin is attached to
   */
  virtual ReturnCode pluginAttach(
      const PropertyList& config,
      void** userdata);

  /**
   * Called for each event definition the plugin is detached from
   */
  virtual void pluginDetach(
      void* userdata);

  /**
   * Produce the next event
   */
  virtual ReturnCode pluginGetNextEvent(
      void* userdata,
      std::string* event_json) = 0;

  /**
   * Returns true if there are pending events, false otherwise
   */
  virtual bool pluginHasPendingEvent(
      void* userdata) = 0;

};

class DynamicSourcePlugin : public SourcePlugin {
public:

  DynamicSourcePlugin(
      evcollect_plugin_getnextevent_fn getnextevent_fn,
      evcollect_plugin_hasnextevent_fn hasnextevent_fn,
      evcollect_plugin_attach_fn attach_fn,
      evcollect_plugin_detach_fn detach_fn,
      evcollect_plugin_init_fn init_fn,
      evcollect_plugin_free_fn free_fn);

  ReturnCode pluginInit(const PluginConfig& cfg) override;
  void pluginFree() override;
  ReturnCode pluginAttach(const PropertyList& config, void** userdata) override;
  void pluginDetach(void* userdata) override;
  ReturnCode pluginGetNextEvent(void* userdata, std::string* data) override;
  bool pluginHasPendingEvent(void* userdata) override;

protected:
  evcollect_plugin_getnextevent_fn getnextevent_fn_;
  evcollect_plugin_hasnextevent_fn hasnextevent_fn_;
  evcollect_plugin_attach_fn attach_fn_;
  evcollect_plugin_detach_fn detach_fn_;
  evcollect_plugin_init_fn init_fn_;
  evcollect_plugin_free_fn free_fn_;
};

class OutputPlugin {
public:

  virtual ~OutputPlugin() = default;

  /**
   * Called when the daemon is started
   */
  virtual ReturnCode pluginInit(const PluginConfig& config);

  /**
   * Called when the daemon is stopped
   */
  virtual void pluginFree();

  /**
   * Called for each event definition the plugin is attached to
   */
  virtual ReturnCode pluginAttach(
      const PropertyList& config,
      void** userdata);

  /**
   * Called for each event definition the plugin is detached from
   */
  virtual void pluginDetach(
      void* userdata);

  /**
   * Emit the next event
   */
  virtual ReturnCode pluginEmitEvent(
      void* userdata,
      const EventData& evdata) = 0;

};

class DynamicOutputPlugin : public OutputPlugin {
public:

  DynamicOutputPlugin(
      evcollect_plugin_emitevent_fn emitevent_fn,
      evcollect_plugin_attach_fn attach_fn,
      evcollect_plugin_detach_fn detach_fn,
      evcollect_plugin_init_fn init_fn,
      evcollect_plugin_free_fn free_fn);

  ReturnCode pluginInit(const PluginConfig& cfg) override;
  void pluginFree() override;
  ReturnCode pluginAttach(const PropertyList& config, void** userdata) override;
  void pluginDetach(void* userdata) override;
  ReturnCode pluginEmitEvent(void* userdata, const EventData& evdata) override;

protected:
  evcollect_plugin_emitevent_fn emitevent_fn_;
  evcollect_plugin_attach_fn attach_fn_;
  evcollect_plugin_detach_fn detach_fn_;
  evcollect_plugin_init_fn init_fn_;
  evcollect_plugin_free_fn free_fn_;
};

ReturnCode loadPlugin(
    PluginContext* ctx,
    std::string plugin_path);

} // namespace evcollect

