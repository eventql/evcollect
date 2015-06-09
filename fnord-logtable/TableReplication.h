/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_LOGTABLE_TABLEREPLICATION_H
#define _FNORD_LOGTABLE_TABLEREPLICATION_H
#include <thread>
#include <fnord-base/stdtypes.h>
#include <fnord-base/uri.h>
#include <fnord-logtable/TableRepository.h>
#include "fnord-http/httprequest.h"
#include "fnord-http/httpconnectionpool.h"

namespace fnord {
namespace logtable {

class TableReplication {
public:

  TableReplication(http::HTTPConnectionPool* http);

  void start();
  void stop();

  void replicateTableFrom(
      RefPtr<TableWriter> table,
      const URI& source_uri);

protected:

  void pull(
      RefPtr<TableWriter> table,
      const URI& source_uri);

  void run();

  uint64_t interval_;
  std::atomic<bool> running_;
  std::thread thread_;

  Vector<Pair<RefPtr<TableWriter>, URI>> targets_;
  http::HTTPConnectionPool* http_;
};

} // namespace logtable
} // namespace fnord

#endif
