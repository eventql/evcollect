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
#include <set>
#include <mutex>
#include <condition_variable>
#include <evcollect/evcollect.h>
#include <evcollect/plugin.h>
#include <evcollect/util/return_code.h>

namespace evcollect {
class SourcePlugin;
class OutputPlugin;

class Service {
public:

  Service(
      const std::string& spool_dir,
      const std::string& plugin_dir);

  ~Service();

  ReturnCode addEvent(const EventBindingConfig* event_binding);
  ReturnCode addTarget(const TargetBindingConfig* target_cfg);

  ReturnCode loadPlugin(const std::string& plugin);
  ReturnCode loadPlugin(bool (*init_fn)(evcollect_ctx_t* ctx));

  const std::string& getSpoolDir() const;
  const std::string& getPluginDir() const;

  ReturnCode run();
  void kill();

protected:

  struct EventSourceBinding {
    SourcePlugin* plugin;
    void* userdata;
  };

  struct EventBinding {
    std::string event_name;
    uint64_t interval_micros;
    std::vector<EventSourceBinding> sources;
    uint64_t next_tick;
  };

  struct TargetBinding {
    OutputPlugin* plugin;
    void* userdata;
  };

  ReturnCode processEvent(EventBinding* binding);

  ReturnCode emitEvent(
      EventBinding* binding,
      uint64_t time,
      const std::string& event_data);

  ReturnCode deliverEvent(const EventData& evdata);

  std::string spool_dir_;
  std::string plugin_dir_;
  PluginMap plugin_map_;
  std::vector<std::unique_ptr<EventBinding>> event_bindings_;
  std::vector<std::unique_ptr<TargetBinding>> targets_;
  std::multiset<
      EventBinding*,
      std::function<bool (EventBinding*, EventBinding*)>> queue_;
  int listen_fd_;
  int wakeup_pipe_[2];
};

} // namespace evcollect