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
#include <string>
#include <set>
#include <evcollect/dispatch.h>
#include <evcollect/plugin.h>
#include <evcollect/util/logging.h>
#include <evcollect/util/time.h>
#include <unistd.h>

namespace evcollect {

namespace {

std::string mergeEvents(const std::string& base, const std::string& overlay) {
  return overlay;
}

} // namespace

Dispatch::Dispatch() :
    queue_([] (EventBinding* a, EventBinding* b) {
      return a->next_tick < b->next_tick;
    }),
    listen_fd_(-1) {
  if (pipe(wakeup_pipe_) < 0) {
    logFatal("pipe() failed");
    abort();
  }
}

Dispatch::~Dispatch() {
  close(wakeup_pipe_[0]);
  close(wakeup_pipe_[1]);
}

void Dispatch::addEventBinding(EventBinding* binding) {
  binding->next_tick = MonotonicClock::now() + binding->interval_micros;
  queue_.insert(binding);
}

void Dispatch::addTargetBinding(TargetBinding* binding) {
  targets_.push_back(binding);
}

ReturnCode Dispatch::emitEvent(
    EventBinding* binding,
    uint64_t time,
    const std::string& event_data) {
  EventData evdata;
  evdata.time = time;
  evdata.event_name = binding->event_name;
  evdata.event_data = event_data;

  logDebug("EMIT: $0 => $1", evdata.event_name, evdata.event_data);
  return deliverEvent(evdata);
}

ReturnCode Dispatch::deliverEvent(const EventData& evdata) {
  auto rc_aggr = ReturnCode::success();
  for (const auto& t : targets_) {
    auto rc = t->plugin->pluginEmitEvent(t->userdata, evdata);
    if (!rc.isSuccess()) {
      rc_aggr = rc;
    }
  }

  return rc_aggr;
}

ReturnCode Dispatch::run() {
  if (queue_.size() == 0) {
    return ReturnCode::success();
  }

  while (true) {
    auto now = MonotonicClock::now();
    auto job = *queue_.begin();
    auto sleep = job->next_tick - now;

    fd_set sleep_fdset;
    FD_ZERO(&sleep_fdset);
    FD_SET(wakeup_pipe_[0], &sleep_fdset);
    if (listen_fd_ > 0) {
      FD_SET(listen_fd_, &sleep_fdset);
    }

    struct timeval sleep_tv;
    sleep_tv.tv_sec = sleep / 1000000;
    sleep_tv.tv_usec = sleep % 1000000;

    int select_rc = select(
        wakeup_pipe_[0] + 1,
        &sleep_fdset,
        NULL,
        NULL,
        &sleep_tv);

    if (select_rc > 0) {
      if (FD_ISSET(wakeup_pipe_[0], &sleep_fdset)) {
        return ReturnCode::success();
      }
      if (listen_fd_ > 0 && FD_ISSET(listen_fd_, &sleep_fdset)) {
        logInfo("Monitor attached");
        continue;
      }
    }

    now = MonotonicClock::now();
    if (job->next_tick > now) {
      continue;
    }

    auto rc = runOnce(job);
    if (!rc.isSuccess()) {
      logError(
          "Error while processing event '$0': $1",
          job->event_name,
          rc.getMessage());
    }

    queue_.erase(queue_.begin());

    now = MonotonicClock::now();
    job->next_tick = job->next_tick + job->interval_micros;
    if (job->next_tick < now) {
      logWarning(
          "Processing event '$0' took longer than the configured " \
          "interval, skipping samples",
          job->event_name);

      job->next_tick = now;
    }

    queue_.insert(job);
  }
}

ReturnCode Dispatch::runOnce(EventBinding* binding) {
  if (binding->sources.empty()) {
    return ReturnCode::success();
  }

  auto now = WallClock::unixMicros();

  std::string event_merged;
  std::string event_buf;
  for (bool cont = true; cont; ) {
    cont = false;
    event_merged.clear();

    for (const auto& src : binding->sources) {
      event_buf.clear();
      {
        auto rc = src.plugin->pluginGetNextEvent(
            src.userdata,
            &event_buf);

        if (!rc.isSuccess()) {
          return rc;
        }
      }

      if (event_merged.empty()) {
        event_merged = event_buf;
      } else {
        event_merged = mergeEvents(event_merged, event_buf);
      }

      if (src.plugin->pluginHasPendingEvent(src.userdata)) {
        cont = true;
      }
    }

    if (!event_merged.empty()) {
      auto rc = emitEvent(binding, now, event_merged);
      if (!rc.isSuccess()) {
        return rc;
      }
    }
  }

  return ReturnCode::success();
}

void Dispatch::kill() {
  char data = 0;
  int rc = write(wakeup_pipe_[1], &data, 1);
  (void) rc;
}

} // namespace evcollect
