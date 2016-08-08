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
#pragma once
#include <string>
#include <set>
#include <mutex>
#include <condition_variable>
#include <evcollect/evcollect.h>
#include <evcollect/plugin.h>
#include <evcollect/util/return_code.h>

namespace evcollect {
class Service {
public:

  static std::unique_ptr<Service> createService(
      const std::string& spool_dir,
      const std::string& plugin_dir);

  virtual ~Service() = default;

  virtual ReturnCode addEvent(const EventConfig* event_binding) = 0;
  virtual ReturnCode addTarget(const TargetConfig* target_cfg) = 0;

  virtual ReturnCode loadPlugin(const std::string& plugin) = 0;
  virtual ReturnCode loadPlugin(bool (*init_fn)(evcollect_ctx_t* ctx)) = 0;

  virtual ReturnCode run() = 0;
  virtual void kill() = 0;

};

} // namespace evcollect

