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
#ifndef _FNORD_HTTP_HTTPRESPONSESTREAM_H
#define _FNORD_HTTP_HTTPRESPONSESTREAM_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/autoref.h>
#include <fnord-http/httpresponse.h>
#include <fnord-http/httpserverconnection.h>

namespace fnord {
namespace http {

class HTTPResponseStream : public RefCounted {
public:
  static const size_t kMaxWriteBufferSize = 4 * 1024 * 1024;

  HTTPResponseStream(HTTPServerConnection* conn);

  /**
   * Write the provided http response (including headers and body) and then
   * immediately finish the response. After calling this method you must not
   * call any of startResponse, writeBodyChunk or finishResponse
   */
  void writeResponse(HTTPResponse res);

  /**
   * Start writing the HTTP response (i.e. write the headers). After calling
   * this method you may call writeBodyChunk zero or more times and then MUST
   * call finishResponse
   */
  void startResponse(const HTTPResponse& resp);

  /**
   * Write a HTTP response body chunk. Only legal if the HTTP response was 
   * previously started by calling "startResponse". You may call this method
   * zero or more times before calling finishResponse
   */
  void writeBodyChunk(const VFSFile& buf);

  /**
   * Finish the http response. Must be called iff the HTTP response was started
   * by calling "startResponse"
   */
  void finishResponse();

  /**
   * Register a callback that will be called after all pending body chunks
   * have been written to the client.
   */
  void onBodyWritten(Function<void ()> callback);

  /**
   * Wait for the reader to consume the outstanding write buffer (i.e. block
   * until the write buffer size has fallen below a threshold.) This is an
   * advisory facility and it is not required to call this method correctness.
   * Should be used to apply "backpressure" to streaming writer to make sure
   * we don't fill up write buffers faster than the reader is reading.
   */
  void waitForReader();

  /**
   * Return the number of outstanding bytes in the write buffer
   */
  size_t bufferSize();

  /**
   * Returns true if the HTTP response was started (i.e. we started writing the
   * status line and some headers
   */
  bool isOutputStarted() const;

protected:

  void onCallbackCompleted();
  void onStateChanged(std::unique_lock<std::mutex>* lk);

  mutable std::mutex mutex_;
  mutable std::condition_variable cv_;
  HTTPServerConnection* conn_;
  bool callback_running_;
  bool headers_written_;
  bool response_finished_;
  Buffer buf_;
  Function<void ()> on_body_written_;
};

}
}
#endif
