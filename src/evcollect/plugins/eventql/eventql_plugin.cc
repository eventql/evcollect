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
#include <thread>
#include <evcollect/plugins/eventql/eventql_plugin.h>
#include <evcollect/util/logging.h>

namespace evcollect {
namespace plugin_eventql {

class EventQLTarget {
public:

  EventQLTarget();

  ReturnCode emitEvent(const EventData& event);

  ReturnCode startUploadThread();
  void stopUploadThread();

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
  bool awaitEvent(EnqueuedEvent* event);

  ReturnCode getTargetTables(
      const EventData& eventdata,
      std::vector<TargetTable>* targets);

  void runUploadThread();
  ReturnCode uploadEvent(const EnqueuedEvent& event);

  std::deque<EnqueuedEvent> queue_;
  mutable std::mutex mutex_;
  mutable std::condition_variable cv_;
  size_t queue_max_length_;
  std::thread thread_;
  bool thread_running_;
  bool thread_shutdown_;
};

EventQLTarget::EventQLTarget() :
    queue_max_length_(1024),
    thread_running_(false) {}

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

  while (queue_.size() >= queue_max_length_) {
    cv_.wait(lk);
  }

  queue_.emplace_back(event);
  cv_.notify_all();

  return ReturnCode::success();
}

bool EventQLTarget::awaitEvent(EnqueuedEvent* event) {
  std::unique_lock<std::mutex> lk(mutex_);

  if (queue_.size() == 0) {
    cv_.wait(lk);
  }

  if (queue_.size() == 0) {
    return false;
  } else {
    *event = queue_.front();
    queue_.pop_front();
    cv_.notify_all();
    return true;
  }
}

ReturnCode EventQLTarget::startUploadThread() {
  if (thread_running_) {
    return ReturnCode::error("RTERROR", "upload thread is already running");
  }

  std::unique_lock<std::mutex> lk(mutex_);
  thread_running_ = true;
  thread_shutdown_ = false;
  thread_ = std::thread(std::bind(&EventQLTarget::runUploadThread, this));
  return ReturnCode::success();
}

void EventQLTarget::stopUploadThread() {
  if (!thread_running_) {
    return;
  }

  thread_shutdown_ = true;
  cv_.notify_all();
  thread_.join();
  thread_running_ = false;
}

void EventQLTarget::runUploadThread() {
  while (true) {
    {
      std::unique_lock<std::mutex> lk(mutex_);
      if (thread_shutdown_) {
        return;
      }
    }

    EnqueuedEvent ev;
    if (!awaitEvent(&ev)) {
      continue;
    }

    auto rc = uploadEvent(ev);
    if (!rc.isSuccess()) {
      logError(
          "error while uploading event to $0/$1: $2", 
          ev.database,
          ev.table,
          rc.getMessage());
    }
  }
}

ReturnCode EventQLTarget::uploadEvent(const EnqueuedEvent& ev) {
  logInfo("upload event: $0/$1 => $2", ev.database, ev.table, ev.data);
  return ReturnCode::success();
}

ReturnCode EventQLPlugin::pluginAttach(
    const PropertyList& config,
    void** userdata) {
  std::unique_ptr<EventQLTarget> target(new EventQLTarget());
  target->startUploadThread();
  *userdata = target.release();
  return ReturnCode::success();
}

void EventQLPlugin::pluginDetach(void* userdata) {
  auto target = static_cast<EventQLTarget*>(userdata);
  target->stopUploadThread();
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

