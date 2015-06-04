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
#include "SSEStreamServlet.h"

namespace fnord {
namespace http {

void SSEStreamServlet::handleHTTPRequest(
      RefPtr<http::HTTPRequestStream> req_stream,
      RefPtr<http::HTTPResponseStream> res_stream) {

  HTTPSSEStream sse(req_stream, res_stream);
  sse.start();

  for (int i = 0; i < 10; ++i) {
    sse.sendEvent(StringUtil::toString(i), Some(String("myevent")));
    usleep(100000);
  }

  sse.finish();
}
}
}
