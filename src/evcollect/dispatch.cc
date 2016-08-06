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
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

namespace {

uint64_t getMonoTime() {

#ifdef __MACH__
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  return std::uint64_t(mts.tv_sec) * 1000000 + mts.tv_nsec / 1000;
#else
  timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
    logFatal("clock_gettime(CLOCK_MONOTONIC) failed");
    abort();
  } else {
    return std::uint64_t(ts.tv_sec) * 1000000 + ts.tv_nsec / 1000;
  }
#endif

}

std::string mergeEvents(const std::string& base, const std::string& overlay) {
  return overlay;
}


} // namespace

namespace evcollect {

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
  binding->next_tick = getMonoTime() + binding->interval_micros;
  queue_.insert(binding);
}

ReturnCode Dispatch::emitEvent(
    EventBinding* binding,
    const std::string& event_data) {
  logInfo("EVENT: $0 => $1", binding->event_name, event_data);
  return ReturnCode::success();
}

ReturnCode Dispatch::run() {
  if (queue_.size() == 0) {
    return ReturnCode::success();
  }

  while (true) {
    auto now = getMonoTime();
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

    now = getMonoTime();
    if (job->next_tick > now) {
      continue;
    }

    auto rc = runOnce(job);
    if (!rc.isSuccess()) {
      logError(
          "Error while fetching event from '$0': $1",
          job->event_name,
          rc.getMessage());
    }

    queue_.erase(queue_.begin());

    now = getMonoTime();
    job->next_tick = job->next_tick + job->interval_micros;
    if (job->next_tick < now) {
      logWarning(
          "Fetching event data for '$0' took longer than the configured " \
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

  std::string event_merged;
  std::string event_buf;
  for (const auto& src : binding->sources) {
    while (src.plugin->pluginHasPendingEvent(src.userdata)) {
      event_buf.clear();
      auto rc = src.plugin->pluginGetNextEvent(
          binding,
          &event_buf);

      if (!rc.isSuccess()) {
        return rc;
      }

      if (binding->collapse_events) {
        event_merged = mergeEvents(event_merged, event_buf);
        break;
      } else {
        auto rc = emitEvent(binding, event_buf);
        if (!rc.isSuccess()) {
          return rc;
        }
      }
    }
  }

  if (binding->collapse_events) {
    emitEvent(binding, event_merged);
  }

  return ReturnCode::success();
}

void Dispatch::kill() {
  char data = 0;
  write(wakeup_pipe_[1], &data, 1);
}

} // namespace evcollect