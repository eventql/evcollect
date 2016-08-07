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
#include <vector>

#ifdef __cplusplus

namespace evcollect {

struct PropertyList {
  std::vector<std::pair<std::string, std::vector<std::string>>> properties;
  bool get(const std::string& key, std::string* out) const;
  size_t get(
      const std::string& key,
      std::vector<std::vector<std::string>>* out) const;
};

struct EventData {
  uint64_t time;
  std::string event_name;
  std::string event_data;
};

} // namespace evcollect

#endif

extern "C" {

typedef void evcollect_ctx_t;
typedef void evcollect_plugin_cfg_t;
typedef void evcollect_plugin_binding_t;
typedef void evcollect_props_t;
typedef void evcollect_event_t;

void evcollect_source_plugin_register(
    const char* plugin_name,
    bool (*get_next_event_fn)(void** userdata, evcollect_event_t* ev),
    bool (*has_next_event_fn)(void** userdata) = nullptr,
    bool (*attach_fn)(const evcollect_plugin_binding_t* cfg, void** userdata) = nullptr,
    bool (*detach_fn)(void** userdata) = nullptr,
    bool (*init_fn)(const evcollect_plugin_cfg_t* cfg) = nullptr,
    void (*free_fn)() = nullptr);

void evcollect_output_plugin_register(
    const char* plugin_name,
    bool (*emit_event_fn)(void** userdata, const evcollect_event_t* ev),
    bool (*attach_fn)(const evcollect_plugin_binding_t* cfg, void** userdata) = nullptr,
    bool (*detach_fn)(void** userdata) = nullptr,
    bool (*init_fn)(const evcollect_plugin_cfg_t* cfg) = nullptr,
    void (*free_fn)() = nullptr);

void evcollect_seterror(evcollect_ctx_t* ctx, const char* error);

void evcollect_event_setdata(
    evcollect_event_t* ev,
    const char* data,
    size_t size);

__attribute__((visibility("default"))) bool evcollect_plugin_init(evcollect_ctx_t* ctx);

}

