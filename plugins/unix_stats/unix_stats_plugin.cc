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
#include <evcollect/evcollect.h>
#include "disk_stats.h"
#include "kernel_stats.h"
#include "process_stats.h"

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

  KernelInfo kernel_info;
  if (!getKernelInfo(kernel_info)) {
    return 0;
  }

  std::vector<ProcessInfo> process_list;
  if (!getProcessInfo(process_list)) {
    return 0;
  }

  std::string json;
  json.append(toJSON(disk_info));
  json.append(toJSON(kernel_info));
  json.append(toJSON(process_list));

  return 1;
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

  return true;
}
