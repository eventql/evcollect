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
#include <deque>
#include <evcollect/plugins/eventql/eventql_plugin.h>

namespace evcollect {
namespace plugin_eventql {

class EventQLTarget {
public:

  EventQLTarget();

  ReturnCode emitEvent(const EventData& event);

protected:

  struct TargetTable {
    std::string database;
    std::string table;
  };

  struct EnqueuedEvent {
    std::string database;
    std::string table;
    std::string data;
  };

  ReturnCode enqueueEvent(const EnqueuedEvent& event);

  ReturnCode getTargetTables(
      const EventData& eventdata,
      std::vector<TargetTable>* targets);

  std::deque<EnqueuedEvent> queue_;
  mutable std::mutex mutex_;
  mutable std::condition_variable cv_;
  size_t queue_max_length_;
  size_t queue_length_;
};

EventQLTarget::EventQLTarget() : queue_max_length_(10), queue_length_(0) {}

ReturnCode EventQLTarget::emitEvent(const EventData& event) {
  std::vector<TargetTable> target_tables;
  {
    auto rc = getTargetTables(event, &target_tables);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  for (const auto& target_table : target_tables) {
    EnqueuedEvent e;
    e.database = target_table.database;
    e.table = target_table.table;
    e.data = event.event_data;

    auto rc = enqueueEvent(e);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  return ReturnCode::success();
}

ReturnCode EventQLTarget::getTargetTables(
    const EventData& eventdata,
    std::vector<TargetTable>* targets) {

  // not yet implemented...
  TargetTable t;
  t.database = "mydb";
  t.table = "mytbl";
  targets->push_back(t);

  return ReturnCode::success();
}

ReturnCode EventQLTarget::enqueueEvent(const EnqueuedEvent& event) {
  std::unique_lock<std::mutex> lk(mutex_);

  if (queue_length_ >= queue_max_length_) {
    return ReturnCode::error("QUEUE_FULL", "EventQL upload queue is full");
  }

  queue_.emplace_back(event);
  ++queue_length_;
  cv_.notify_all();

  return ReturnCode::success();
}

ReturnCode EventQLPlugin::pluginAttach(
    const PropertyList& config,
    void** userdata) {
  std::unique_ptr<EventQLTarget> target(new EventQLTarget());
  *userdata = target.release();
  return ReturnCode::success();
}

void EventQLPlugin::pluginDetach(void* userdata) {
  auto target = static_cast<EventQLTarget*>(userdata);
  delete target;
}

ReturnCode EventQLPlugin::pluginEmitEvent(
    void* userdata,
    const EventData& evdata) {
  auto target = static_cast<EventQLTarget*>(userdata);
  return target->emitEvent(evdata);
}

} // namespace plugins_eventql
} // namespace evcollect

