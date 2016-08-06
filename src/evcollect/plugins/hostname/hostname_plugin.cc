/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
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
#include <netdb.h>
#include <unistd.h>
#include <evcollect/util/stringutil.h>
#include "hostname_plugin.h"

namespace evcollect {
namespace plugin_hostname {

ReturnCode HostnamePlugin::pluginGetNextEvent(
    const EventBinding* event,
    void* userdata,
    std::string* event_json) {
  std::string hostname;
  std::string hostname_fqdn;

  hostname.resize(1024);
  if (gethostname(&hostname[0], hostname.size()) == -1) {
    return ReturnCode::error("SYSCALL_FAILED", "gethostname() failed");
  } else {
    hostname.resize(strlen(hostname.data()));
  }

  struct hostent* h = gethostbyname(hostname.c_str());
  hostname_fqdn = std::string(h->h_name);

  *event_json = StringUtil::format(
      R"({ "hostname": "$0", "hostname_fqdn": "$1" })",
      StringUtil::jsonEscape(hostname),
      StringUtil::jsonEscape(hostname_fqdn));

  return ReturnCode::success();
}

bool HostnamePlugin::pluginHasPendingEvent(
    const EventBinding* event,
    void* userdata) {
  return true;
}

} // namespace plugins_hostname
} // namespace evcollect
