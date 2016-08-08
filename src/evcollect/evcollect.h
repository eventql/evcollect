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

/**
 * This file contains the common public C and C++ API as well as the C plugin
 * api
 */

extern "C" {

typedef void evcollect_ctx_t;
typedef void evcollect_plugin_cfg_t;
typedef void evcollect_event_t;

void evcollect_seterror(evcollect_ctx_t* ctx, const char* error);

enum evcollect_loglevel {
  EVCOLLECT_LOG_FATAL = 10,
  EVCOLLECT_LOG_EMERGENCY = 9,
  EVCOLLECT_LOG_ALERT = 8,
  EVCOLLECT_LOG_CRITICAL = 7,
  EVCOLLECT_LOG_ERROR = 6,
  EVCOLLECT_LOG_WARNING = 5,
  EVCOLLECT_LOG_NOTICE = 4,
  EVCOLLECT_LOG_INFO = 3,
  EVCOLLECT_LOG_DEBUG = 2,
  EVCOLLECT_LOG_TRACE = 1
};

void evcollect_log(
    evcollect_loglevel level,
    const char* msg);

bool evcollect_plugin_getcfg(
    const evcollect_plugin_cfg_t* cfg,
    const char* key,
    const char** value);

bool evcollect_plugin_getcfgv(
    const evcollect_plugin_cfg_t* cfg,
    const char* key,
    size_t i,
    size_t j,
    const char** value);

void evcollect_event_getname(
    const evcollect_event_t* ev,
    const char** data,
    size_t* size);

void evcollect_event_setname(
    evcollect_event_t* ev,
    const char* data,
    size_t size);

void evcollect_event_getdata(
    const evcollect_event_t* ev,
    const char** data,
    size_t* size);

void evcollect_event_setdata(
    evcollect_event_t* ev,
    const char* data,
    size_t size);

typedef bool (*evcollect_plugin_getnextevent_fn)(
    evcollect_ctx_t* ctx,
    void* userdata,
    evcollect_event_t* ev);

typedef bool (*evcollect_plugin_hasnextevent_fn)(
    evcollect_ctx_t* ctx,
    void* userdata);

typedef bool (*evcollect_plugin_emitevent_fn)(
    evcollect_ctx_t* ctx,
    void* userdata,
    const evcollect_event_t* ev);

typedef bool (*evcollect_plugin_attach_fn)(
    evcollect_ctx_t* ctx,
    const evcollect_plugin_cfg_t* cfg,
    void** userdata);

typedef bool (*evcollect_plugin_detach_fn)(
    evcollect_ctx_t* ctx,
    void* userdata);

typedef bool (*evcollect_plugin_init_fn)(
    evcollect_ctx_t* ctx);

typedef void (*evcollect_plugin_free_fn)(
    evcollect_ctx_t* ctx);

void evcollect_source_plugin_register(
    evcollect_ctx_t* ctx,
    const char* plugin_name,
    evcollect_plugin_getnextevent_fn getnextevent_fn,
    evcollect_plugin_hasnextevent_fn hasnextevent_fn = nullptr,
    evcollect_plugin_attach_fn attach_fn = nullptr,
    evcollect_plugin_detach_fn detach_fn = nullptr,
    evcollect_plugin_init_fn init_fn = nullptr,
    evcollect_plugin_free_fn free_fn = nullptr);

void evcollect_output_plugin_register(
    evcollect_ctx_t* ctx,
    const char* plugin_name,
    evcollect_plugin_emitevent_fn emitevent_fn,
    evcollect_plugin_attach_fn attach_fn = nullptr,
    evcollect_plugin_detach_fn detach_fn = nullptr,
    evcollect_plugin_init_fn init_fn = nullptr,
    evcollect_plugin_free_fn free_fn = nullptr);

bool __evcollect_plugin_init(
    evcollect_ctx_t* ctx);

#define EVCOLLECT_PLUGIN_INIT(N) \
    extern "C" __attribute__((visibility("default"))) bool plugin_ ## N ## _init(evcollect_ctx_t* ctx)

} // extern "C"

#ifdef __cplusplus

namespace evcollect {

struct EventData {
  uint64_t time;
  std::string event_name;
  std::string event_data;
};

struct PropertyList {
  std::vector<std::pair<std::string, std::vector<std::string>>> properties;
  bool get(const std::string& key, std::string* out) const;
  bool get(const std::string& key, const char** out) const;
  bool getv(const std::string& key, size_t i, size_t j, const char** out) const;
  size_t get(
      const std::string& key,
      std::vector<std::vector<std::string>>* out) const;
};

struct EventSourceConfig {
  std::string plugin_name;
  std::string plugin_value;
  PropertyList properties;
};

struct EventConfig {
  std::string event_name;
  uint64_t interval_micros;
  std::vector<EventSourceConfig> sources;
};

struct TargetConfig {
  std::string plugin_name;
  std::string plugin_value;
  PropertyList properties;
};

} // namespace evcollect

#endif
