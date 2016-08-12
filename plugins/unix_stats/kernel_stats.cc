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
#include "kernel_stats.h"
#include "util/time.h"

#if __linux__
#include <sys/sysinfo.h>
#endif

#if __APPLE__
#include <sys/sysctl.h>
#endif


namespace evcollect {
namespace plugin_unix_stats {

bool getKernelInfo(KernelInfo kernel_info) {
#if __linux__
  struct sysinfo info;
  if (sysinfo(&info) == -1) {
    //evcollect_seterror(ctx, "sysinfo failed");
    return false;
  }

  /* load average for the last 1, 5 and 15 minutes */
  switch (sizeof(info.loads) / sizeof(info.loads[0])) {
    case 3:
      kernel_info.load_avg.min15 = info_loads[2];

    case 2:
      kernel_info.load_avg.min15 = info_loads[1];

    case 1:
      kernel_info.load_avg.min5 = info_loads[0];
  }

  kernel_info.uptime = uptime;

#elif __APPLE__

  /* load averages */
  {
    struct loadavg loadinfo;
    size_t size = sizeof(loadinfo);

    if (sysctlbyname("vm.loadavg", &loadinfo, &size, NULL, 0) == -1) {
      //evcollect_seterror(
      //    ctx,
      //    StringUtil::format("sysctlbyname failed: $0", strerror(errno)).c_str());
      return false;
    }

    /* load average for the last 1, 5 and 15 minutes */
    switch (sizeof(loadinfo.ldavg) / sizeof(fixpt_t)) {
      case 3:
        kernel_info.load_avg.min15 = (double) loadinfo.ldavg[2] / loadinfo.fscale;

      case 2:
        kernel_info.load_avg.min15 = (double) loadinfo.ldavg[1] / loadinfo.fscale;

      case 1:
        kernel_info.load_avg.min5 = (double) loadinfo.ldavg[0] / loadinfo.fscale;
    }
  }

  /* uptime */
  {
    struct timeval t;
    size_t size = sizeof(t);

    if (sysctlbyname("kern.boottime", &t, &size, NULL, 0) == -1) {
      //evcollect_seterror(
      //    ctx,
      //    StringUtil::format("sysctlbyname failed: $0", strerror(errno)).c_str());
      return false;
    }

    UnixTime now;
    kernel_info.uptime = (now.unixMicros() / kMicrosPerSecond) - t.tv_sec;
  }

  /* kernel version */
  {
    int mib[2] = {CTL_KERN, KERN_VERSION};
    char version[256];
    size_t len = 256;
    if (sysctl(mib, 2, version, &len, NULL, 0) == -1) {
      return false;
    }

    printf("kernel version %s", version);

  }
#endif

  return true;
}

std::string toJSON(KernelInfo kernel_info) {

}

} //namespace plugin_unix_stats
} //namespace evcollect


