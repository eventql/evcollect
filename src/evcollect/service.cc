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
#include <regex>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <evcollect/service.h>
#include <evcollect/config.h>
#include <evcollect/plugin.h>
#include <evcollect/logfile.h>
#include <evcollect/util/logging.h>
#include <evcollect/util/time.h>

namespace evcollect {

namespace {

std::string mergeEvents(const std::string& base, const std::string& overlay) {
  return overlay;
}

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

class ServiceImpl : public Service {
public:

  ServiceImpl(
      const std::string& spool_dir,
      const std::string& plugin_dir);

  ~ServiceImpl() override;

  ReturnCode addEvent(const EventConfig* event_binding) override;
  ReturnCode addTarget(const TargetConfig* target_cfg) override;

  ReturnCode loadPlugin(const std::string& plugin) override;
  ReturnCode loadPlugin(bool (*init_fn)(evcollect_ctx_t* ctx)) override;

  ReturnCode run() override;
  void kill() override;

protected:

  ReturnCode processEvent(EventBinding* binding);

  ReturnCode emitEvent(
      EventBinding* binding,
      uint64_t time,
      const std::string& event_data);

  ReturnCode deliverEvent(const EventData& evdata);

  std::string spool_dir_;
  std::string plugin_dir_;
  PluginMap plugin_map_;
  PluginContext plugin_ctx_;
  std::vector<std::unique_ptr<EventBinding>> event_bindings_;
  std::vector<std::unique_ptr<TargetBinding>> targets_;
  std::multiset<
      EventBinding*,
      std::function<bool (EventBinding*, EventBinding*)>> queue_;
  int listen_fd_;
  int wakeup_pipe_[2];
};

ServiceImpl::ServiceImpl(
    const std::string& spool_dir,
    const std::string& plugin_dir) :
    spool_dir_(spool_dir),
    plugin_dir_(plugin_dir),
    plugin_map_(spool_dir, plugin_dir),
    queue_([] (EventBinding* a, EventBinding* b) {
      return a->next_tick < b->next_tick;
    }),
    listen_fd_(-1) {
  plugin_ctx_.plugin_map = &plugin_map_;
  LogfileSourcePlugin::registerPlugin(&plugin_map_);

  if (pipe(wakeup_pipe_) < 0) {
    logFatal("pipe() failed");
    abort();
  }
}

ServiceImpl::~ServiceImpl() {
  for (auto& binding : event_bindings_) {
    for (auto& source : binding->sources) {
      source.plugin->pluginDetach(source.userdata);
    }
  }

  for (auto& binding : targets_) {
    binding->plugin->pluginDetach(binding->userdata);
  }

  close(wakeup_pipe_[0]);
  close(wakeup_pipe_[1]);
}

ReturnCode ServiceImpl::addEvent(const EventConfig* binding) {
  std::unique_ptr<EventBinding> ev_binding(new EventBinding());
  ev_binding->event_name = binding->event_name;
  ev_binding->interval_micros = binding->interval_micros;

  for (const auto& source : binding->sources) {
    EventSourceBinding ev_source;
    {
      auto rc = plugin_map_.getSourcePlugin(
          source.plugin_name,
          &ev_source.plugin);

      if (!rc.isSuccess()) {
        return rc;
      }
    }

    {
      auto rc = ev_source.plugin->pluginAttach(
          source.properties,
          &ev_source.userdata);

      if (!rc.isSuccess()) {
        return rc;
      }
    }

    ev_binding->sources.emplace_back(ev_source);
  }

  ev_binding->next_tick = MonotonicClock::now() + ev_binding->interval_micros;
  queue_.insert(ev_binding.get());
  event_bindings_.emplace_back(std::move(ev_binding));
  return ReturnCode::success();
}

ReturnCode ServiceImpl::addTarget(const TargetConfig* binding) {
  std::unique_ptr<TargetBinding> trgt_binding(new TargetBinding());

  {
    auto rc = plugin_map_.getOutputPlugin(
        binding->plugin_name,
        &trgt_binding->plugin);

    if (!rc.isSuccess()) {
      return rc;
    }
  }

  {
    auto rc = trgt_binding->plugin->pluginAttach(
        binding->properties,
        &trgt_binding->userdata);

    if (!rc.isSuccess()) {
      return rc;
    }
  }

  targets_.emplace_back(std::move(trgt_binding));
  return ReturnCode::success();
}

ReturnCode ServiceImpl::loadPlugin(const std::string& plugin) {
  std::vector<std::string> path_candidates;
  std::string plugin_name;
  if (StringUtil::isShellSafe(plugin)) {
    plugin_name = plugin;
  } else {
    path_candidates.emplace_back(plugin);
    std::regex plugin_path_regex(".*plugin_(\\w+)(\\.\\w+)?");
    std::smatch m;
    if (!std::regex_match(plugin, m, plugin_path_regex)) {
      return ReturnCode::error(
          "EPLUGIN",
          StringUtil::format(
              "invalid plugin filename '$0' -- should be plugin_<name>.so",
              plugin));
    }

    plugin_name = m[1];
  }

  path_candidates.emplace_back(plugin_dir_ + "/" + plugin);
  path_candidates.emplace_back(plugin_dir_ + "/plugin_" + plugin + ".so");

  for (const auto& path : path_candidates) {
    struct stat s;
    if (stat(path.c_str(), &s) != 0) {
      continue;
    }

    auto rc = evcollect::loadPlugin(&plugin_ctx_, plugin_name, path);
    if (rc.isSuccess()) {
      return rc;
    } else {
      return ReturnCode::error(
          "EPLUGIN",
          StringUtil::format(
              "error while loading plugin '$0': $1",
              plugin,
              rc.getMessage()));
    }
  }

  return ReturnCode::error(
      "EPLUGIN",
      StringUtil::format(
          "plugin not found -- tried $0",
          StringUtil::join(path_candidates, ", ")));
}

ReturnCode ServiceImpl::loadPlugin(bool (*init_fn)(evcollect_ctx_t* ctx)) {
  PluginContext plugin_ctx;
  plugin_ctx.plugin_map = &plugin_map_;

  auto rc = evcollect::loadPlugin(&plugin_ctx_, init_fn);
  if (rc.isSuccess()) {
    return rc;
  } else {
    return ReturnCode::error(
        "EPLUGIN",
        StringUtil::format(
            "error while loading plugin: $0",
            rc.getMessage()));
  }
}

ReturnCode ServiceImpl::emitEvent(
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

ReturnCode ServiceImpl::deliverEvent(const EventData& evdata) {
  auto rc_aggr = ReturnCode::success();
  for (const auto& t : targets_) {
    auto rc = t->plugin->pluginEmitEvent(t->userdata, evdata);
    if (!rc.isSuccess()) {
      rc_aggr = rc;
    }
  }

  return rc_aggr;
}

ReturnCode ServiceImpl::run() {
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

    auto rc = processEvent(job);
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

ReturnCode ServiceImpl::processEvent(EventBinding* binding) {
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

void ServiceImpl::kill() {
  char data = 0;
  int rc = write(wakeup_pipe_[1], &data, 1);
  (void) rc;
}

} // namespace

std::unique_ptr<Service> Service::createService(
    const std::string& spool_dir,
    const std::string& plugin_dir) {
  return std::unique_ptr<Service>(new ServiceImpl(spool_dir, plugin_dir));
}


} // namespace evcollect
