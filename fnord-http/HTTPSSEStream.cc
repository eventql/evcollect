/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *   Copyright (c) 2015 Laura Schlimmer
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "HTTPSSEStream.h"

namespace fnord {
namespace http {

HTTPSSEStream::HTTPSSEStream(
  RefPtr<http::HTTPRequestStream> req_stream,
  RefPtr<http::HTTPResponseStream> res_stream) :
  req_stream_(req_stream),
  res_stream_(res_stream) {}

void HTTPSSEStream::start() {
  req_stream_->readBody();
  res_.populateFromRequest(req_stream_->request());
  res_.setStatus(kStatusOK);
  res_.addHeader("Content-Type", "text/event-stream");
  res_.addHeader("Cache-Control", "no-cache");
  res_.addHeader("Access-Control-Allow-Origin", "*");
  res_stream_->startResponse(res_);
}

void HTTPSSEStream::sendEvent(
    const String& data,
    const Option<String>& event_type) {
  sendEvent(data.data(), data.size(), event_type);
}

void HTTPSSEStream::sendEvent(
    const Buffer& data,
    const Option<String>& event_type) {
  sendEvent(data.data(), data.size(), event_type);
}

void HTTPSSEStream::sendEvent(
    const void* event_data,
    size_t event_size,
    const Option<String>& event_type) {
  Buffer buf;
  buf.reserve(event_size + 1024);

  if (!event_type.isEmpty()) {
    buf.append("event: ");
    buf.append(event_type.get());
    buf.append("\n");
  }

  buf.append("data: ");
  buf.append(event_data, event_size);
  buf.append("\n\n");

  res_stream_->writeBodyChunk(buf);
}

void HTTPSSEStream::finish() {
  res_stream_->finishResponse();
}

const HTTPResponse HTTPSSEStream::response() const {
  return res_;
}


}
}
