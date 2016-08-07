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
#include <stdio.h>
#include <sys/statvfs.h>
#include <evcollect/util/stringutil.h>
#include "unix_stats_plugin.h"

#ifdef linux
#include <mntent.h>
#elif __APPLE__
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#endif

namespace evcollect {
namespace plugin_unix_stats {

ReturnCode UnixStatsPlugin::pluginGetNextEvent(
    void* userdata,
    std::string* event_json) {
  std::string hostname;
  std::string hostname_fqdn;

  event_json->append("([");

  auto mount_info = getMountInfo();
  for (size_t i = 0; i < mount_info.size(); ++i) {
    if (i > 0) {
      event_json->append(",");
    }

    struct statvfs buf;
    if (statvfs(mount_info[i].mount_point.c_str(), &buf) == -1) {
      continue;
    }

    auto total = (double) (buf.f_blocks * buf.f_frsize) / (1024 * 1024 * 1024);
    auto available = (double) (buf.f_bavail * buf.f_frsize) / (1024 * 1024 * 1024);
    auto used = total - available;

    auto ifree = buf.f_favail;
    auto iused = buf.f_files - ifree;

    event_json->append(StringUtil::format(
        R"({ 
          "filesystem": "$0",
          "total": $1,
          "used": $2,
          "available": $3,
          "capacity": $4,
          "iused": $5,
          "ifree": $6,
          "mount_point": "$7"
        })",
        StringUtil::jsonEscape(mount_info[i].device),
        total,
        used,
        available,
        used / total,
        iused,
        ifree,
        StringUtil::jsonEscape(mount_info[i].mount_point)));
  }

  event_json->append("])");

  return ReturnCode::success();
}

bool UnixStatsPlugin::pluginHasPendingEvent(
    void* userdata) {
  return false;
}

std::vector<UnixStatsPlugin::MountInfo> UnixStatsPlugin::getMountInfo() {
std::vector<UnixStatsPlugin::MountInfo> mount_info;

#ifdef linux
  auto file = setmentent("/etc/fstab", "r");
  
  while (auto mntent = getmntent(file)) {
    printf("filesystemt: %s, mounted on: %s", mntent.mnt_fsname, mntent.mnt_dir);
  }

    


#elif __APPLE__

  struct statfs *mntbuf;
  auto mntsize = getmntinfo(&mntbuf, MNT_NOWAIT);
  for (size_t i = 0; i < mntsize; ++i) {
    UnixStatsPlugin::MountInfo mn_info = {
      .device = mntbuf[i].f_mntfromname,
      .mount_point = mntbuf[i].f_mntonname
    };

    mount_info.emplace_back(mn_info);
  }

#endif

  return mount_info;
}

} // namespace plugin_unix_stats
} // namespace evcollect

