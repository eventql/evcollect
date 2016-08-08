/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Christian Parpart <christianparpart@googlemail.io>
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
#include <evcollect/config.h>

namespace evcollect {

ReturnCode loadConfig(
    const std::string& config_file_path,
    ProcessConfig* conf) {
  conf->load_plugins.push_back("./plugins/hostname/.libs/plugin_hostname.so");
  conf->load_plugins.push_back("./plugins/eventql/.libs/plugin_eventql.so");

  {
    // XXX: event sys.alive interval 1s
    conf->event_bindings.emplace_back();
    auto& b = conf->event_bindings.back();
    b.event_name = "sys.alive";
    b.interval_micros = 1000000;

    // XXX: source plugin hostname
    b.sources.emplace_back();
    auto& s = b.sources.back();
    s.plugin_name = "hostname";
  }

  {
    // XXX: event logs.access_log interval 1s
    conf->event_bindings.emplace_back();
    auto& b = conf->event_bindings.back();
    b.event_name = "logs.access_log";
    b.interval_micros = 1000000;
    // XXX: source plugin logfile logfile "/tmp/log" regex /(?<fuu>[^\\|]*)?(?<bar>.*)/
    b.sources.emplace_back();
    auto& s = b.sources.back();
    s.plugin_name = "logfile";
    s.properties.properties.emplace_back(
        std::make_pair(
            "logfile",
            std::vector<std::string> { "/tmp/log" }));

    s.properties.properties.emplace_back(
        std::make_pair(
            "regex",
            std::vector<std::string> { "(?<fuu>[^\\|]*)?(?<bar>.*)" }));
  }

  {
    // XXX: target "eventql1" plugin eventql
    conf->target_bindings.emplace_back();
    auto& b = conf->target_bindings.back();
    b.plugin_name = "eventql";

    // XXX: route logs.access_log "test/logs.access_log"
    b.properties.properties.emplace_back(
        std::make_pair(
            "route",
            std::vector<std::string> { "logs.access_log", "test/logs.access_log" }));

    // XXX: route sys.alive "test/sys.alive"
    b.properties.properties.emplace_back(
        std::make_pair(
            "route",
            std::vector<std::string> { "sys.alive", "test/sys.alive" }));

    // XXX: route sys.alive "test/sys.alive.rollup"
    b.properties.properties.emplace_back(
        std::make_pair(
            "route",
            std::vector<std::string> { "sys.alive", "test/sys.alive.rollup" }));
  }

  return ReturnCode::success();
}

bool PropertyList::get(const std::string& key, std::string* out) const {
  for (const auto& p : properties) {
    if (p.first != key) {
      continue;
    }

    if (p.second.empty()) {
      continue;
    }

    *out = p.second.front();
    return true;
  }

  return false;
}

bool PropertyList::get(const std::string& key, const char** out) const {
  for (const auto& p : properties) {
    if (p.first != key) {
      continue;
    }

    if (p.second.empty()) {
      continue;
    }

    *out = p.second.front().c_str();
    return true;
  }

  return false;
}

bool PropertyList::getv(
    const std::string& key,
    size_t i,
    size_t j,
    const char** out) const {
  for (const auto& p : properties) {
    if (p.first != key) {
      continue;
    }

    if (i > 0) {
      --i;
      continue;
    }

    if (j + 1 > p.second.size()) {
      return false;
    } else {
      *out = p.second[j].c_str();
      return true;
    }
  }

  return false;
}

size_t PropertyList::get(
    const std::string& key,
    std::vector<std::vector<std::string>>* out) const {
  size_t cnt = 0;

  for (const auto& p : properties) {
    if (p.first == key) {
      out->push_back(p.second);
      ++cnt;
    }
  }

  return cnt;
}

} // namespace evcollect
