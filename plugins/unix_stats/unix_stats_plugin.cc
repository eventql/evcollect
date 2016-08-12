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
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <sys/statvfs.h>
#include <evcollect/evcollect.h>
#include "disk_stats.h"
#include "kernel_stats.h"
#include "util/stringutil.h"
#include "util/time.h"

#if __linux__
#include <fstream>
#include <mntent.h>
#include <dirent.h>
#include <sys/sysinfo.h>
#endif

#if __APPLE__
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <pwd.h>
#endif

namespace evcollect {
namespace plugin_unix_stats {

int getEvent(
    evcollect_ctx_t* ctx,
    void* userdata,
    evcollect_event_t* ev) {

  std::vector<DiskInfo> disk_info;
  if (!getDiskInfo(disk_info)) {
    return 0;
  }

  plugin_unix_stats::KernelInfo kernel_info;
  if (!getKernelInfo(kernel_info)) {
    return false;
  }

  return 1;
 // return (!getUptimeEvent(ctx, userdata, ev) ||
 //         !getLoadAvgEvent(ctx, userdata, ev) ||
 //         !getDiskUsageEvent(ctx, userdata, ev) ||
 //         !getProcessesEvent(ctx, userdata, ev));
}

} // namespace plugin_unix_stats
} // namespace evcollect

EVCOLLECT_PLUGIN_INIT(unix_stats) {
  evcollect_source_plugin_register(
      ctx,
      "unix.stats",
      &evcollect::plugin_unix_stats::getEvent,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL);
  

  //evcollect_source_plugin_register(
  //    ctx,
  //    "unix.uptime",
  //    &evcollect::plugin_unix_stats::getUptimeEvent);

  //evcollect_source_plugin_register(
  //    ctx,
  //    "unix.load_avg",
  //    &evcollect::plugin_unix_stats::getLoadAvgEvent);

  //evcollect_source_plugin_register(
  //    ctx,
  //    "unix.disk_usage",
  //    &evcollect::plugin_unix_stats::getDiskUsageEvent);

  //evcollect_source_plugin_register(
  //    ctx,
  //    "unix.processes",
  //    &evcollect::plugin_unix_stats::getProcessesEvent);
  return true;
}
