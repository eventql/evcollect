/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
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
#include <unistd.h>
#include <string.h>
#include <sys/statvfs.h>
#include <evcollect/util/stringutil.h>
#include "unix_stats_plugin.h"

namespace evcollect {
namespace plugin_unix_stats {

ReturnCode UnixStatsPlugin::pluginGetNextEvent(
    void* userdata,
    std::string* event_json) {
  std::string hostname;
  std::string hostname_fqdn;



  *event_json = StringUtil::format(
      R"({ "test": "$0", "blah": "$1" })",
      StringUtil::jsonEscape(hostname),
      StringUtil::jsonEscape(hostname_fqdn));

  return ReturnCode::success();
}

bool UnixStatsPlugin::pluginHasPendingEvent(
    void* userdata) {
  return false;
}

std::vector<UnixStatsPlugin::MountInfo> UnixStatsPlugin::getMountInfo() {
  UnixStatsPlugin::MountInfo mount_info = {
    .device = "test",
    .mount_point = "test",
    .fs_type = UnixStatsPlugin::fsType::EXT3
  };

  std::vector<UnixStatsPlugin::MountInfo> infos;
  infos.emplace_back(mount_info);
  return infos;
}

} // namespace plugin_unix_stats
} // namespace evcollect

