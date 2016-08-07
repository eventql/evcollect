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
#include <string.h>
#include <curl/curl.h>
#include <evcollect/plugins/eventql/eventql_plugin.h>
#include <evcollect/util/logging.h>
#include <evcollect/util/base64.h>

namespace evcollect {
namespace plugin_eventql {

class EventQLTarget {
public:

  EventQLTarget(
      const std::string& hostname,
      uint16_t port);

  ~EventQLTarget();

  void addRoute(
      const std::string& event_name_match,
      const std::string& target);

  void setAuthToken(const std::string& auth_token);
  void setCredentials(
      const std::string& username,
      const std::string& password);

  ReturnCode emitEvent(const EventData& event);

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
};

EventQLTarget::EventQLTarget(
    const std::string& hostname,
    uint16_t port) :
    hostname_(hostname),
    port_(port),
    queue_max_length_(1024),
    thread_running_(false),
    curl_(nullptr) {
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

ReturnCode EventQLTarget::emitEvent(const EventData& event) {
  for (const auto& route : routes_) {
    if (route.event_name_match != event.event_name) {
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
    e.data = event.event_data;

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

ReturnCode EventQLPlugin::pluginAttach(
    const PropertyList& config,
    void** userdata) {
  std::string hostname = "localhost";
  uint16_t port = 9175;

  std::string hostname_opt;
  if (config.get("port", &hostname_opt)) {
    hostname = hostname_opt;
  }

  std::string port_opt;
  if (config.get("port", &port_opt)) {
    try {
      port = std::stoul(port_opt);
    } catch (...) {
      return ReturnCode::error("EINVAL", "invalid port");
    }
  }

  std::unique_ptr<EventQLTarget> target(new EventQLTarget(hostname, port));

  std::string username;
  std::string password;
  config.get("username", &username);
  config.get("password", &password);
  if (!username.empty() || !password.empty()) {
    target->setCredentials(username, password);
  }

  std::string auth_token;
  if (config.get("auth_token", &auth_token)) {
    target->setAuthToken(auth_token);
  }

  std::vector<std::vector<std::string>> route_cfg;
  config.get("route", &route_cfg);
  for (const auto& route : route_cfg) {
    if (route.size() != 2) {
      return ReturnCode::error(
          "EINVAL",
          "invalid number of arguments to route. " \
          "format is: route <event> <target>");
    }

    target->addRoute(route[0], route[1]);
  }

  target->startUploadThread();
  *userdata = target.release();
  return ReturnCode::success();
}

void EventQLPlugin::pluginDetach(void* userdata) {
  auto target = static_cast<EventQLTarget*>(userdata);
  target->stopUploadThread();
  delete target;
}

ReturnCode EventQLPlugin::pluginEmitEvent(
    void* userdata,
    const EventData& evdata) {
  auto target = static_cast<EventQLTarget*>(userdata);
  return target->emitEvent(evdata);
}

} // namespace plugins_eventql
} // namespace evcollect

