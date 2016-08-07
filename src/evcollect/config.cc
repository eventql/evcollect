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
  {
    conf->event_bindings.emplace_back();
    auto& b = conf->event_bindings.back();
    b.event_name = "sys.alive";
    b.interval_micros = 1000000;
    b.sources.emplace_back();
    auto& s = b.sources.back();
    s.plugin_name = "hostname";
  }
  {
    conf->event_bindings.emplace_back();
    auto& b = conf->event_bindings.back();
    b.event_name = "logs.access_log";
    b.interval_micros = 1000000;
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
    conf->target_bindings.emplace_back();
    auto& b = conf->target_bindings.back();
    b.plugin_name = "eventql";

    b.properties.properties.emplace_back(
        std::make_pair(
            "route",
            std::vector<std::string> { "logs.access_log", "test/logs.access_log" }));

    b.properties.properties.emplace_back(
        std::make_pair(
            "route",
            std::vector<std::string> { "sys.alive", "test/sys.alive" }));

    b.properties.properties.emplace_back(
        std::make_pair(
            "route",
            std::vector<std::string> { "sys.alive", "test/sys.alive.rollup" }));
  }

  return ReturnCode::success();
}

} // namespace evcollect
