/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_TSDB_TSDBSERVLET_H
#define _FNORD_TSDB_TSDBSERVLET_H
#include "fnord-http/httpservice.h"
#include <fnord-base/random.h>
#include <fnord-tsdb/TSDBNode.h>

namespace fnord {
namespace tsdb {

class TSDBServlet : public fnord::http::StreamingHTTPService {
public:

  TSDBServlet(TSDBNode* node);

  void handleHTTPRequest(
      RefPtr<http::HTTPRequestStream> req_stream,
      RefPtr<http::HTTPResponseStream> res_stream);

protected:

  void insertRecord(
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      URI* uri);

  void insertRecordsBatch(
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      URI* uri);

  void insertRecordsReplication(
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      URI* uri);

  void listChunks(
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      URI* uri);

  void listFiles(
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      URI* uri);

  void fetchDerivedDataset(
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      URI* uri);

  void fetchChunk(
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      RefPtr<http::HTTPResponseStream> res_stream,
      URI* uri);

  void fetchPartitionInfo(
      const http::HTTPRequest* req,
      http::HTTPResponse* res,
      URI* uri);

  TSDBNode* node_;
  Random rnd_;
};

}
}
#endif
