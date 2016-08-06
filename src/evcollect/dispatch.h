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
#include <evcollect/util/return_code.h>

namespace evcollect {

class SourcePlugin;

struct EventSourceBinding {
  SourcePlugin* plugin;
  void* userdata;
};

struct EventBinding {
  std::string event_name;
  uint64_t interval_micros;
  bool collapse_events;
  std::vector<EventSourceBinding> sources;
  uint64_t next_tick;
};

class Dispatch {
public:

  Dispatch();
  ~Dispatch();

  void addEventBinding(EventBinding* binding);

  ReturnCode run();
  void kill();

protected:

  ReturnCode runOnce(EventBinding* binding);
  ReturnCode emitEvent(EventBinding* binding, const std::string& event_data);

  std::multiset<
      EventBinding*,
      std::function<bool (EventBinding*, EventBinding*)>> queue_;

  int listen_fd_;
  int wakeup_pipe_[2];
};

} // namespace evcollect
