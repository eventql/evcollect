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
#include <evcollect/util/stringutil.h>
#include <evcollect/util/time.h>

#if __linux__
#include <regex>
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

struct MountInfo {
  std::string device;
  std::string mount_point;
};

static std::vector<MountInfo> getMountInfo() {
std::vector<MountInfo> mount_info;
#ifdef __linux__

  auto file = setmntent(MOUNTED, "r");
  while (auto mntent = getmntent(file)) {
    MountInfo mn_info = {
      .device = mntent->mnt_fsname,
      .mount_point = mntent->mnt_dir
    };

    mount_info.emplace_back(mn_info);
  }

#elif __APPLE__

  struct statfs *mntbuf;
  auto mntsize = getmntinfo(&mntbuf, MNT_NOWAIT);
  for (auto i = 0; i < mntsize; ++i) {
    MountInfo mn_info = {
      .device = mntbuf[i].f_mntfromname,
      .mount_point = mntbuf[i].f_mntonname
    };

    mount_info.emplace_back(mn_info);
  }

#else
#error "unsupported os" 
#endif

  return mount_info;
}

bool getDiskUsageEvent(
    evcollect_ctx_t* ctx,
    void* userdata,
    evcollect_event_t* ev) {
  std::string evdata;

  evdata.append(R"({"disk_stats": [)");
  auto mount_info = getMountInfo();
  for (size_t i = 0; i < mount_info.size(); ++i) {
    if (i > 0) {
      evdata.append(",");
    }

    struct statvfs buf;
    if (statvfs(mount_info[i].mount_point.c_str(), &buf) == -1) {
      continue;
    }

    auto total =  (buf.f_blocks * buf.f_frsize);
    auto available =  (buf.f_bavail * buf.f_frsize);
    auto used = total - available;
    auto capacity = total > 0 ? used / total : 1;
    auto ifree = buf.f_favail;
    auto iused = buf.f_files - ifree;

    evdata.append(StringUtil::format(
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
        capacity * 100,
        iused,
        ifree,
        StringUtil::jsonEscape(mount_info[i].mount_point)));
  }

  evdata.append("]}");
  evcollect_event_setdata(ev, evdata.data(), evdata.size());
  return true;
}

bool getLoadAvgEvent(
    evcollect_ctx_t* ctx,
    void* userdata,
    evcollect_event_t* ev) {
  std::string evdata;

#if __linux__
  struct sysinfo info;
  if (sysinfo(&info) == -1) {
    evcollect_seterror(ctx, "sysinfo failed");
    return false;
  }

  /* load average for the last 1, 5 and 15 minutes */
  evdata.append(R"("load_avg" : [)");

  for (size_t i = 0; i < sizeof(info.loads) / sizeof(info.loads[0]); ++i) {
    if (i > 0) {
      evdata.append(",");
    }

    evdata.append(StringUtil::format(
        "{$0: $1}",
        i,
        info.loads[i]));
  }

  evdata.append(StringUtil::format(R"(],{"procs": $0})", info.procs));
  evdata.append(StringUtil::format(R"(,{"freeram": $0})", info.freeram));
  evdata.append(StringUtil::format(R"(,{"freeswap": $0})", info.freeswap));

#elif __APPLE__
  struct loadavg loadinfo;
  size_t size = sizeof(loadinfo);

  if (sysctlbyname("vm.loadavg", &loadinfo, &size, NULL, 0) == -1) {
    evcollect_seterror(
        ctx,
        StringUtil::format("sysctlbyname failed: $0", strerror(errno)).c_str());
    return false;
  }

  /* load average for the last 1, 5 and 15 minutes */
  evdata.append(R"("load_avg" : [)");

  for (size_t i = 0; i < sizeof(loadinfo.ldavg) / sizeof(fixpt_t); ++i) {
    if (i > 0) {
      evdata.append(",");
    }

    evdata.append(StringUtil::format(
        "{$0: $1}",
        i,
        (double) loadinfo.ldavg[i] / loadinfo.fscale));
  }

#endif

  evdata.append("}");
  evcollect_event_setdata(ev, evdata.data(), evdata.size());
  return true;
}

bool getUptimeEvent(
    evcollect_ctx_t* ctx,
    void* userdata,
    evcollect_event_t* ev) {
  std::string evdata;
#if __linux__
    struct sysinfo info;
    if (sysinfo(&info) == -1) {
      evcollect_seterror(ctx, "sysinfo failed");
      return false;
    }

    auto uptime = info.uptime;

#elif __APPLE__
    struct timeval t;
    size_t size = sizeof(t);

    if (sysctlbyname("kern.boottime", &t, &size, NULL, 0) == -1) {
      evcollect_seterror(
          ctx,
          StringUtil::format("sysctlbyname failed: $0", strerror(errno)).c_str());
      return false;
    }

    UnixTime now;
    auto uptime = (now.unixMicros() / kMicrosPerSecond) - t.tv_sec;
#endif

  evdata.append(StringUtil::format(
      R"({"days": $0, "hours": $1, "minutes": $2, "seconds": $3})",
      uptime / kSecondsPerDay,
      uptime / kSecondsPerHour,
      uptime / kSecondsPerMinute,
      uptime));

  evcollect_event_setdata(ev, evdata.data(), evdata.size());
  return true;
}

bool getProcessesEvent(
    evcollect_ctx_t* ctx,
    void* userdata,
    evcollect_event_t* ev) {
  std::string evdata;

#if __linux__
  auto dir = opendir("/proc/");
  if (!dir) {
    evcollect_seterror(
          ctx,
          "opendir failed");
          //StringUtil::format("opendir failed: $0", strerror(errno)).c_str());
    return false;
  }

  for (;;) {
    auto entry = readdir(dir);
    if (!entry) {
      break;
    }

    if (entry->d_type == DT_DIR) {
    //  continue;
    }

    if (StringUtil::isNumber(entry->d_name)) {

    }

    //auto file = fopen(
    //    StringUtil::format("/proc/$0/stat", entry->d_name).c_str(),
    //    "r");
    //if (!file) {
    //  evcollect_seterror(
    //      ctx,
    //      "fopen failed");
    //  return false;
    //}

    //char content[2048];
    //if (!fgets(content, 2048, file)) {
    //  evcollect_seterror(
    //      ctx,
    //      "fgets failed");
    //  return false;
    //}

    //std::regex rgx("\\d");
    //std::smatch match;
    //if (!regex_search(static_cast<std::string>(content), match, rgx)) {
    //  evcollect_seterror(
    //      ctx,
    //      "regex_search failed");
    //  return false;
    //}

    //printf("match %s", match[0]);
  }

#elif __APPLE__
  size_t len = 0;
  int mib[3] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL};
  if (sysctl(mib, 3, NULL, &len, NULL, 0) == -1) {
    evcollect_seterror(
          ctx,
          StringUtil::format("sysctl failed: $0", strerror(errno)).c_str());
    return false;
  }

  struct kinfo_proc *info;
  info = static_cast<kinfo_proc*>(malloc(len));
  if (sysctl(mib, 3, info, &len, NULL, 0) == -1) {
    evcollect_seterror(
          ctx,
          StringUtil::format("sysctl failed: $0", strerror(errno)).c_str());
    free(info);
    return false;
  }

  int count = len / sizeof(kinfo_proc);
  for (int i = 0; i < count; ++i) {
    pid_t pid = info[i].kp_proc.p_pid;
    if (pid == 0) {
      continue;
    }

    uid_t uid = info[i].kp_eproc.e_ucred.cr_uid;
    struct passwd *user = getpwuid(uid);
    const char* username = user ? user->pw_name : "";

    evdata.append(StringUtil::format(
        R"({"pid": $0, "uid": $1, "username": "$2", "parent": $3, "group": $4})",
        pid,
        uid,
        username,
        info[i].kp_eproc.e_ppid,
        info[i].kp_eproc.e_pgid));
  }

  free(info);
#endif

  evcollect_event_setdata(ev, evdata.data(), evdata.size());
  return true;
}

bool getEvent(
    evcollect_ctx_t* ctx,
    void* userdata,
    evcollect_event_t* ev) {
  return (!getUptimeEvent(ctx, userdata, ev) ||
          !getLoadAvgEvent(ctx, userdata, ev) ||
          !getDiskUsageEvent(ctx, userdata, ev) ||
          !getProcessesEvent(ctx, userdata, ev));
}

} // namespace plugin_unix_stats
} // namespace evcollect

EVCOLLECT_PLUGIN_INIT(unix_stats) {
  evcollect_source_plugin_register(
      ctx,
      "unix.stats",
      &evcollect::plugin_unix_stats::getEvent);

  evcollect_source_plugin_register(
      ctx,
      "unix.uptime",
      &evcollect::plugin_unix_stats::getUptimeEvent);

  evcollect_source_plugin_register(
      ctx,
      "unix.load_avg",
      &evcollect::plugin_unix_stats::getLoadAvgEvent);

  evcollect_source_plugin_register(
      ctx,
      "unix.disk_usage",
      &evcollect::plugin_unix_stats::getDiskUsageEvent);

  evcollect_source_plugin_register(
      ctx,
      "unix.processes",
      &evcollect::plugin_unix_stats::getProcessesEvent);
  return true;
}
