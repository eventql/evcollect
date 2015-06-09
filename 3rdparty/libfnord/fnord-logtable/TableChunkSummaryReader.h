/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_LOGTABLE_TABLECHUNKSUMMARYREADER_H
#define _FNORD_LOGTABLE_TABLECHUNKSUMMARYREADER_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/option.h>
#include <fnord-base/autoref.h>
#include <fnord-base/random.h>
#include <fnord-base/io/FileLock.h>
#include <fnord-base/io/mmappedfile.h>
#include <fnord-base/util/binarymessagereader.h>
#include <fnord-base/util/binarymessagewriter.h>
#include <fnord-msg/MessageSchema.h>
#include <fnord-msg/MessageObject.h>

namespace fnord {
namespace logtable {

class TableChunkSummaryReader {
public:

  TableChunkSummaryReader(const String& filename);

  Option<Buffer> getSummaryData(const String& summary_name) const;

  template <typename T>
  Option<T> getSummary(const String& summary_name) const;

  bool getSummaryData(
      const String& summary_name,
      void** data,
      size_t* size) const;

protected:
  HashMap<String, Pair<uint64_t, uint64_t>> offsets_;
  io::MmappedFile mmap_;
  size_t body_offset_;
};

template <typename T>
Option<T> TableChunkSummaryReader::getSummary(
    const String& summary_name) const {
  auto data = getSummaryData(summary_name);

  if (data.isEmpty()) {
    return None<T>();
  }

  util::BinaryMessageReader reader(data.get().data(), data.get().size());
  T summary;
  summary.decode(&reader);
  return Some(summary);
}


}
}
#endif
