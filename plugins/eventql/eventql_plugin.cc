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
#include <deque>
#include <thread>
#include <condition_variable>
#include <string.h>
#include <curl/curl.h>
#include <evcollect/evcollect.h>
#include <evcollect/util/logging.h>
#include <evcollect/util/base64.h>
#include <evcollect/util/time.h>
#include <evcollect/util/return_code.h>

namespace evcollect {
namespace plugin_eventql {

class EventQLTarget {
public:

  static const size_t kDefaultHTTPTimeoutMicros = 30 * kMicrosPerSecond;
  static const size_t kDefaultMaxQueueLength = 8192;

  EventQLTarget(
      const std::string& hostname,
      uint16_t port);

  ~EventQLTarget();

  void addRoute(
      const std::string& event_name_match,
      const std::string& target);

  void setHTTPTimeout(uint64_t usecs);
  void setMaxQueueLength(size_t queue_len);

  void setAuthToken(const std::string& auth_token);
  void setCredentials(
      const std::string& username,
      const std::string& password);

  ReturnCode emitEvent(
    const std::string& event_name,
    const std::string& event_data);

  ReturnCode startUploadThread();
  void stopUploadThread();

protected:

  struct TargetTable {
    std::string database;
    std::string table;
  };

  struct EnqueuedEvent {
    std::string database;
    std::string table;
    std::string data;
  };

  struct EventRouting {
    std::string event_name_match;
    std::string target;
  };

  ReturnCode enqueueEvent(const EnqueuedEvent& event);
  bool awaitEvent(EnqueuedEvent* event);
  ReturnCode uploadEvent(const EnqueuedEvent& event);

  std::string hostname_;
  uint16_t port_;
  std::string username_;
  std::string password_;
  std::string auth_token_;
  std::deque<EnqueuedEvent> queue_;
  mutable std::mutex mutex_;
  mutable std::condition_variable cv_;
  size_t queue_max_length_;
  std::thread thread_;
  bool thread_running_;
  bool thread_shutdown_;
  std::vector<EventRouting> routes_;
  CURL* curl_;
  uint64_t http_timeout_;
};

EventQLTarget::EventQLTarget(
    const std::string& hostname,
    uint16_t port) :
    hostname_(hostname),
    port_(port),
    queue_max_length_(kDefaultMaxQueueLength),
    thread_running_(false),
    curl_(nullptr),
    http_timeout_(kDefaultHTTPTimeoutMicros) {
  curl_ = curl_easy_init();
}

EventQLTarget::~EventQLTarget() {
  if (curl_) {
    curl_easy_cleanup(curl_);
  }
}

void EventQLTarget::addRoute(
    const std::string& event_name_match,
    const std::string& target) {
  EventRouting r;
  r.event_name_match = event_name_match;
  r.target = target;
  routes_.emplace_back(r);
}

void EventQLTarget::setAuthToken(const std::string& auth_token) {
  auth_token_ = auth_token;
}

void EventQLTarget::setCredentials(
    const std::string& username,
    const std::string& password) {
  username_ = username;
  password_ = password;
}

void EventQLTarget::setHTTPTimeout(uint64_t usecs) {
  http_timeout_ = usecs;
}

void EventQLTarget::setMaxQueueLength(size_t queue_len) {
  queue_max_length_ = queue_len;
}

ReturnCode EventQLTarget::emitEvent(
    const std::string& event_name,
    const std::string& event_data) {
  for (const auto& route : routes_) {
    if (route.event_name_match != event_name) {
      continue;
    }

    auto target = StringUtil::split(route.target, "/");
    if (target.size() != 2) {
      return ReturnCode::error(
          "EINVAL",
          "invalid target specification. " \
          "format is: database/table");
    }

    EnqueuedEvent e;
    e.database = target[0];
    e.table = target[1];
    e.data = event_data;

    auto rc = enqueueEvent(e);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  return ReturnCode::success();
}

ReturnCode EventQLTarget::enqueueEvent(const EnqueuedEvent& event) {
  std::unique_lock<std::mutex> lk(mutex_);

  while (queue_.size() >= queue_max_length_) {
    cv_.wait(lk);
  }

  queue_.emplace_back(event);
  cv_.notify_all();

  return ReturnCode::success();
}

bool EventQLTarget::awaitEvent(EnqueuedEvent* event) {
  std::unique_lock<std::mutex> lk(mutex_);

  if (queue_.size() == 0) {
    cv_.wait(lk);
  }

  if (queue_.size() == 0) {
    return false;
  } else {
    *event = queue_.front();
    queue_.pop_front();
    cv_.notify_all();
    return true;
  }
}

ReturnCode EventQLTarget::startUploadThread() {
  if (thread_running_) {
    return ReturnCode::error("RTERROR", "upload thread is already running");
  }

  auto upload_thread = [this] () {
    while (true) {
      {
        std::unique_lock<std::mutex> lk(mutex_);
        if (thread_shutdown_) {
          return;
        }
      }

      EnqueuedEvent ev;
      if (!awaitEvent(&ev)) {
        continue;
      }

      auto rc = uploadEvent(ev);
      if (!rc.isSuccess()) {
        logError(
            "error while uploading event to $0/$1: $2", 
            ev.database,
            ev.table,
            rc.getMessage());
      }
    }
  };

  std::unique_lock<std::mutex> lk(mutex_);
  thread_running_ = true;
  thread_shutdown_ = false;
  thread_ = std::thread(upload_thread);
  return ReturnCode::success();
}

void EventQLTarget::stopUploadThread() {
  if (!thread_running_) {
    return;
  }

  thread_shutdown_ = true;
  cv_.notify_all();
  thread_.join();
  thread_running_ = false;
}

namespace {
size_t curl_write_cb(void* data, size_t size, size_t nmemb, std::string* s) {
  size_t pos = s->size();
  size_t len = pos + size * nmemb;
  s->resize(len);
  memcpy((char*) s->data() + pos, data, size * nmemb);
  return size * nmemb;
}
}

ReturnCode EventQLTarget::uploadEvent(const EnqueuedEvent& ev) {
  auto url = StringUtil::format(
      "http://$0:$1/api/v1/tables/insert",
      hostname_,
      port_);

  std::string body;
  body += "[{ ";
  body += StringUtil::format(
      "\"database\": \"$0\",",
      StringUtil::jsonEscape(ev.database));
  body += StringUtil::format(
      "\"table\": \"$0\"",
      StringUtil::jsonEscape(ev.table));
  body += "\"data\":" + ev.data;
  body += "}]";

  if (!curl_) {
    return ReturnCode::error("EIO", "curl_init() failed");
  }

  struct curl_slist* req_headers = NULL;
  req_headers = curl_slist_append(
      req_headers,
      "Content-Type: application/json; charset=utf-8");

  if (!auth_token_.empty()) {
    auto hdr = "Authorization: Token " + auth_token_;
    req_headers = curl_slist_append(req_headers, hdr.c_str());
  }

  if (!username_.empty() || !password_.empty()) {
    std::string hdr = "Authorization: Basic ";
    hdr += Base64::encode(username_ + ":" + password_);
    req_headers = curl_slist_append(req_headers, hdr.c_str());
  }

  std::string res_body;
  curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl_, CURLOPT_TIMEOUT_MS, http_timeout_ / kMicrosPerMilli);
  curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, body.c_str());
  curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, curl_write_cb);
  curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &res_body);
  CURLcode curl_res = curl_easy_perform(curl_);
  curl_slist_free_all(req_headers);
  if (curl_res != CURLE_OK) {
    return ReturnCode::error(
        "EIO",
        "http request failed: %s",
        curl_easy_strerror(curl_res));
  }

  long http_res_code = 0;
  curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &http_res_code);

  switch (http_res_code) {
    case 201:
      return ReturnCode::success();
    case 400:
      return ReturnCode::error(
          "EINVAL",
          "http error: %li -- %.*s",
          http_res_code,
          res_body.size(),
          res_body.data());
    case 403:
    case 401:
      return ReturnCode::error(
          "EACCESS",
          "auth error: %li -- %.*s",
          http_res_code,
          res_body.size(),
          res_body.data());
    default:
      return ReturnCode::error(
          "EIO",
          "http error: %li -- %.*s",
          http_res_code,
          res_body.size(),
          res_body.data());
  }
}

bool pluginAttach(
    evcollect_ctx_t* ctx,
    const evcollect_plugin_cfg_t* cfg,
    void** userdata) {
  std::string hostname = "localhost";
  uint16_t port = 9175;

  const char* hostname_opt;
  if (evcollect_plugin_getcfg(cfg, "hostname", &hostname_opt)) {
    hostname = std::string(hostname_opt);
  }

  const char* port_opt;
  if (evcollect_plugin_getcfg(cfg, "port", &port_opt)) {
    try {
      port = std::stoul(port_opt);
    } catch (...) {
      evcollect_seterror(ctx, "invalid port");
      return false;
    }
  }

  std::unique_ptr<EventQLTarget> target(new EventQLTarget(hostname, port));

  std::string username;
  const char* username_opt;
  if (evcollect_plugin_getcfg(cfg, "username", &username_opt)) {
    username = std::string(username_opt);
  }

  std::string password;
  const char* password_opt;
  if (evcollect_plugin_getcfg(cfg, "password", &password_opt)) {
    password = std::string(password_opt);
  }

  if (!username.empty() || !password.empty()) {
    target->setCredentials(username, password);
  }


  const char* auth_token_opt;
  if (evcollect_plugin_getcfg(cfg, "auth_token", &auth_token_opt)) {
    target->setAuthToken(std::string(auth_token_opt));
  }

  const char* http_timeout_opt;
  if (evcollect_plugin_getcfg(cfg, "http_timeput", &http_timeout_opt)) {
    uint64_t http_timeout;
    try {
      http_timeout = std::stoull(http_timeout_opt);
    } catch (...) {
      evcollect_seterror(ctx, "invalid value for http_timeout");
      return false;
    }

    target->setHTTPTimeout(http_timeout);
  }

  const char* queue_maxlen_opt;
  if (evcollect_plugin_getcfg(cfg, "http_timeput", &queue_maxlen_opt)) {
    uint64_t queue_maxlen;
    try {
      queue_maxlen = std::stoull(queue_maxlen_opt);
    } catch (...) {
      evcollect_seterror(ctx, "invalid value for queue_maxlen");
      return false;
    }

    target->setMaxQueueLength(queue_maxlen);
  }


//
//  std::vector<std::vector<std::string>> route_cfg;
//  config.get("route", &route_cfg);
//  for (const auto& route : route_cfg) {
//    if (route.size() != 2) {
//      return ReturnCode::error(
//          "EINVAL",
//          "invalid number of arguments to route. " \
//          "format is: route <event> <target>");
//    }
//
//    target->addRoute(route[0], route[1]);
//  }
//

  target->startUploadThread();
  *userdata = target.release();
  return true;
}

bool pluginDetach(evcollect_ctx_t* ctx, void* userdata) {
  auto target = static_cast<EventQLTarget*>(userdata);
  target->stopUploadThread();
  delete target;
  return true;
}

bool pluginEmitEvent(
    evcollect_ctx_t* ctx,
    void* userdata,
    const evcollect_event_t* event) {
  auto target = static_cast<EventQLTarget*>(userdata);

  const char* ev_name;
  size_t ev_name_len;
  evcollect_event_getname(event, &ev_name, &ev_name_len);

  const char* ev_data;
  size_t ev_data_len;
  evcollect_event_getdata(event, &ev_data, &ev_data_len);

  auto rc = target->emitEvent(
      std::string(ev_name, ev_name_len),
      std::string(ev_data, ev_data_len));

  if (rc.isSuccess()) {
    return true;
  } else {
    evcollect_seterror(ctx, rc.getMessage().c_str());
    return false;
  }
}

} // namespace plugins_eventql
} // namespace evcollect

bool __evcollect_plugin_init(evcollect_ctx_t* ctx) {
  evcollect_output_plugin_register(
      ctx,
      "output",
      &evcollect::plugin_eventql::pluginEmitEvent,
      &evcollect::plugin_eventql::pluginAttach,
      &evcollect::plugin_eventql::pluginDetach);

  return true;
}

