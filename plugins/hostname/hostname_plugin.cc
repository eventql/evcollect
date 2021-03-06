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
#include <string.h>
#include <util/stringutil.h>
#include <evcollect/evcollect.h>

namespace evcollect {
namespace plugin_hostname {

int getEvent(
    evcollect_ctx_t* ctx,
    void* userdata,
    evcollect_event_t* ev) {
  std::string hostname;
  std::string hostname_fqdn;

  hostname.resize(1024);
  if (gethostname(&hostname[0], hostname.size()) == -1) {
    evcollect_seterror(ctx, "gethostname() failed");
    return false;
  } else {
    hostname.resize(strlen(hostname.data()));
  }

  struct hostent* h = gethostbyname(hostname.c_str());
  if (h) {
    hostname_fqdn = std::string(h->h_name);
  }

  auto evdata = StringUtil::format(
      R"({ "hostname": "$0", "hostname_fqdn": "$1" })",
      StringUtil::jsonEscape(hostname),
      StringUtil::jsonEscape(hostname_fqdn));

  evcollect_event_setdata(ev, evdata.data(), evdata.size());
  return true;
}

} // namespace plugins_hostname
} // namespace evcollect

EVCOLLECT_PLUGIN_INIT(hostname) {
  evcollect_source_plugin_register(
      ctx,
      "hostname",
      &evcollect::plugin_hostname::getEvent,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL);

  return true;
}

