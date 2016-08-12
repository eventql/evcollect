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

namespace evcollect {
namespace plugin_unix_stats {


bool getProcessInfo(std::vector<ProcessInfo> process_info) {
//  std::string evdata;
//
//#if __linux__
//  auto dir = opendir("/proc/");
//  if (!dir) {
//    evcollect_seterror(
//          ctx,
//          "opendir failed");
//          //StringUtil::format("opendir failed: $0", strerror(errno)).c_str());
//    return false;
//  }
//
//  evdata.append("[");
//  size_t i = 0;
//  for (;;) {
//    auto entry = readdir(dir);
//    if (!entry) {
//      break;
//    }
//
//    if (entry->d_type != DT_DIR || (!StringUtil::isNumber(entry->d_name))) {
//      continue;
//    }
//
//    std::ifstream file(
//        StringUtil::format("/proc/$0/stat", entry->d_name).c_str(),
//        std::ifstream::in);
//
//    if (!file.is_open()) {
//      return false;
//    }
//
//    if (i++ > 0) {
//      evdata.append(",");
//    }
//
//    evdata.append("{");
//
//    size_t j = 0;
//    std::string buf;
//    while (!file.eof()) {
//      char c;
//      file.get(c);
//      if (isspace(c)) {
//        switch (j++) {
//          case 0: /* process id */
//            evdata.append(StringUtil::format(R"("pid": $0)", buf));
//            break;
//
//          case 1:  /* filename of the executable */
//            evdata.append(StringUtil::format(R"(,"name": "$0")", buf));
//            break;
//
//          case 2: /* process state */
//            evdata.append(StringUtil::format(R"(,"state": "$0")", buf));
//            break;
//
//          case 3: /* parent PID */
//            evdata.append(StringUtil::format(R"(,"ppid": $0)", buf));
//            break;
//
//          case 4: /* group ID */
//            evdata.append(StringUtil::format(R"(,"pgrp": $0)", buf));
//            break;
//
//          case 13: /* time in user mode */
//            evdata.append(StringUtil::format(R"(,"utime": $0)", buf));
//            break;
//
//          case 14: /* time in kernel mode */
//            evdata.append(StringUtil::format(R"(,"stime": $0)", buf));
//            break;
//
//          case 18: /* nice value */
//            evdata.append(StringUtil::format(R"(,"nice": $0)", buf));
//            break;
//
//          case 21: /* starttime */
//            evdata.append(StringUtil::format(R"(,"starttime": $0)", buf));
//            break;
//
//          case 22: /* virtual memory size */
//            evdata.append(StringUtil::format(R"(,"vsize": $0)", buf));
//            break;
//
//          default:
//            break;
//        }
//
//        buf.clear();
//      } else {
//        buf.push_back(c);
//      }
//
//    }
//
//    evdata.append("}");
//  }
//
//  evdata.append("]");
//
//#elif __APPLE__
//  size_t len = 0;
//  int mib[3] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL};
//  if (sysctl(mib, 3, NULL, &len, NULL, 0) == -1) {
//    evcollect_seterror(
//          ctx,
//          StringUtil::format("sysctl failed: $0", strerror(errno)).c_str());
//    return false;
//  }
//
//  struct kinfo_proc* info;
//  info = static_cast<kinfo_proc*>(malloc(len));
//  if (sysctl(mib, 3, info, &len, NULL, 0) == -1) {
//    free(info);
//    evcollect_seterror(
//          ctx,
//          StringUtil::format("sysctl failed: $0", strerror(errno)).c_str());
//    return false;
//  }
//
//  int count = len / sizeof(kinfo_proc);
//  for (int i = 0; i < count; ++i) {
//    pid_t pid = info[i].kp_proc.p_pid;
//    if (pid == 0) {
//      continue;
//    }
//
//    evdata.append(StringUtil::format(
//        R"({
//            "pid": $0,
//            "parent": $1,
//            "group": $2,
//            "nice": $3,
//            "starttime": $4,
//            "state": "$5"})",
//        pid,
//        info[i].kp_eproc.e_ppid,
//        info[i].kp_eproc.e_pgid,
//        (uint64_t) info[i].kp_proc.p_nice,
//        (uint64_t) info[i].kp_proc.p_un.__p_starttime.tv_sec,
//        (uint64_t) info[i].kp_proc.p_stat));
//  }
//
//  free(info);
//#endif
//
//  evcollect_event_setdata(ev, evdata.data(), evdata.size());
//  return true;
  return true;
}

std::string toJSON(std::vector<ProcessInfo> process_info) {
  std::string json;
  return json;
}

} //namespace plugin_unix_stats
} //namespace evcollect


