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

#include "process_stats.h"
#include "util/stringutil.h"
#if __linux__
#include <fstream>
//#include <mntent.h>
#include <dirent.h>
//#include <sys/sysinfo.h>
#endif

#if __APPLE__
#include <sys/sysctl.h>
#endif

namespace evcollect {
namespace plugin_unix_stats {


bool getProcessInfo(std::vector<ProcessInfo> process_info) {
#if __linux__
  auto dir = opendir("/proc/");
  if (!dir) {
    //      //StringUtil::format("opendir failed: $0", strerror(errno)).c_str());
    return false;
  }

  for (;;) {
    auto entry = readdir(dir);
    /* end of directory */
    if (!entry) {
      break;
    }

    /* skip if not one of the /proc/{pid} directories */
    if (entry->d_type != DT_DIR || (!StringUtil::isNumber(entry->d_name))) {
      continue;
    }

    auto file_name = StringUtil::format("/proc/$0/stat", entry->d_name);
    std::ifstream file(
        file_name.c_str(),
        std::ifstream::in);

    if (!file.is_open()) {
      return false;
    }

    size_t i = 0;
    std::string buf;
    ProcessInfo info;
    while (!file.eof()) {
      char c;
      file.get(c);
      if (isspace(c)) {
        switch (i++) {
          case 0: /* process id */
            info.pid = std::stoull(buf);
            break;

          case 1:  /* filename of the executable */
            info.name = buf;
            break;

          case 2: /* process state */
            info.state = buf;
            break;

          case 3: /* parent PID */
            info.ppid = std::stoull(buf);
            break;

          case 4: /* group ID */
            info.pgrp = std::stoull(buf);
            break;

          case 13: /* time in user mode */
            info.utime = std::stoull(buf);
            break;

          case 14: /* time in kernel mode */
            info.stime = std::stoull(buf);
            break;

          case 18: /* nice value */
            info.nice = std::stoul(buf);
            break;

          case 21: /* starttime */
            info.starttime = std::stoull(buf);
            break;

          case 22: /* virtual memory size */
            info.vsize = std::stoull(buf);
            break;

          default:
            break;
        }

        buf.clear();
      } else {
        buf.push_back(c);
      }
    }

    process_info.emplace_back(info);
  }

  closedir(dir);

#elif __APPLE__
  size_t len = 0;
  int name[3] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL};
  /* size of kinfo_proc */
  if (sysctl(name, 3, NULL, &len, NULL, 0) == -1) {
    //      StringUtil::format("sysctl failed: $0", strerror(errno)).c_str());
    return false;
  }

  struct kinfo_proc* info;
  info = static_cast<kinfo_proc*>(malloc(len));
  if (sysctl(name, 3, info, &len, NULL, 0) == -1) {
    free(info);
    //      StringUtil::format("sysctl failed: $0", strerror(errno)).c_str());
    return false;
  }

  int count = len / sizeof(kinfo_proc);
  for (int i = 0; i < count; ++i) {
    pid_t pid = info[i].kp_proc.p_pid;
    if (pid == 0) {
      continue;
    }

    struct ProcessInfo pinfo;
    pinfo.pid = pid;
    pinfo.ppid = info[i].kp_eproc.e_ppid;
    pinfo.pgrp = info[i].kp_eproc.e_pgid;
    pinfo.nice = (uint64_t) info[i].kp_proc.p_nice;
    pinfo.starttime = (uint64_t) info[i].kp_proc.p_un.__p_starttime.tv_sec;
    pinfo.state = info[i].kp_proc.p_stat;

    process_info.emplace_back(pinfo);
  }

  free(info);
#endif
  return true;
}

std::string toJSON(std::vector<ProcessInfo> process_info) {
  std::string json;
  return json;
}

} //namespace plugin_unix_stats
} //namespace evcollect


