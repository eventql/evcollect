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

#include <sys/statvfs.h>
#include <unistd.h>
#include <stdio.h>
#include <disk_stats.h>
#include <evcollect/util/stringutil.h>
#if __linux__
#include <mntent.h>
#endif

#if __APPLE__
#include <sys/mount.h>
#endif

namespace evcollect {
namespace plugin_unix_stats {

std::vector<MountInfo> getMountInfo() {
  std::vector<MountInfo> mount_info;

#if __linux__

  auto file = setmntent(MOUNTED, "r");
  while (auto mntent = getmntent(file)) {
    MountInfo mn_info = {
      .device = mntent->mnt_fsname,
      .mount_point = mntent->mnt_dir
    };

    mount_info.emplace_back(mn_info);
  }
  //FIXME close file

#elif __APPLE__
  struct statfs* mntbuf;
  auto mntsize = getmntinfo(&mntbuf, MNT_NOWAIT);
  for (auto i = 0; i < mntsize; ++i) {
    MountInfo mn_info = {
      .device = mntbuf[i].f_mntfromname,
      .mount_point = mntbuf[i].f_mntonname
    };

    mount_info.emplace_back(mn_info);
  }

#endif

  return mount_info;
}


bool getDiskInfo(std::vector<DiskInfo> disk_info) {
  auto mount_info = getMountInfo();

  for (size_t i = 0; i < mount_info.size(); ++i) {
    struct statvfs buf;
    if (statvfs(mount_info[i].mount_point.c_str(), &buf) == -1) {
      continue;
    }

    DiskInfo di;
    di.total = buf.f_blocks * buf.f_frsize;
    di.available = buf.f_bavail * buf.f_frsize;
    di.used = di.total - di.available;
    di.capacity = di.total > 0 ? (di.used / di.total) * 100 : 100;
    di.ifree = buf.f_favail;
    di.iused = buf.f_files - di.ifree;

    disk_info.emplace_back(di);
  }

  return true;
}

std::string toJSON(std::vector<DiskInfo> disk_info) {
  std::string json;
  json.append(R"("disk_info": [)");

  for (size_t i = 0; i < disk_info.size(); ++i) {
    if (i > 0) {
      json.append(",");
    }

    json.append(StringUtil::format(R"({
      "total": $0,
      "available": $1,
      "used": $2,
      "capacity": $3,
      "ifree": $4,
      "iused": $5})",
      disk_info[i].total,
      disk_info[i].available,
      disk_info[i].used,
      disk_info[i].capacity,
      disk_info[i].ifree,
      disk_info[i].used
    ));
  }

  json.append("]");
  return json;
}

} //namespace plugin_unix_stats
} //namespace evcollect
