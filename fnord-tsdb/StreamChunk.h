/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_TSDB_STREAMCHUNK_H
#define _FNORD_TSDB_STREAMCHUNK_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/option.h>
#include <fnord-base/datetime.h>
#include <fnord-base/util/binarymessagereader.h>
#include <fnord-base/util/binarymessagewriter.h>
#include <fnord-msg/MessageSchema.h>
#include <fnord-tsdb/StreamProperties.h>
#include <fnord-tsdb/RecordSet.h>
#include <fnord-tsdb/TSDBNodeRef.h>
#include <fnord-tsdb/PartitionInfo.pb.h>

namespace fnord {
namespace tsdb {

struct StreamChunkState {
  String stream_key;
  RecordSet::RecordSetState record_state;
  HashMap<uint64_t, uint64_t> repl_offsets;
  void encode(util::BinaryMessageWriter* writer) const;
  void decode(util::BinaryMessageReader* reader);
};

class StreamChunk : public RefCounted {
public:

  static RefPtr<StreamChunk> create(
      const String& streamchunk_key,
      const String& stream_key,
      RefPtr<StreamProperties> config,
      TSDBNodeRef* node);

  static RefPtr<StreamChunk> reopen(
      const String& streamchunk_key,
      const StreamChunkState& state,
      RefPtr<StreamProperties> config,
      TSDBNodeRef* node);

  static String streamChunkKeyFor(
      const String& stream_key,
      DateTime time,
      Duration partition_size);

  static String streamChunkKeyFor(
      const String& stream_key,
      DateTime time,
      const StreamProperties& properties);

  static Vector<String> streamChunkKeysFor(
      const String& stream_key,
      DateTime from,
      DateTime until,
      const StreamProperties& properties);

  void insertRecord(
      uint64_t record_id,
      const Buffer& record);

  void insertRecords(const Vector<RecordRef>& records);

  void compact();
  void replicate();
  void buildIndexes();

  Vector<String> listFiles() const;

  PartitionInfo partitionInfo() const;

  Buffer fetchDerivedDataset(const String& dataset_name);

protected:

  StreamChunk(
      const String& streamchunk_key,
      const String& stream_key,
      RefPtr<StreamProperties> config,
      TSDBNodeRef* node);

  StreamChunk(
      const String& streamchunk_key,
      const StreamChunkState& state,
      RefPtr<StreamProperties> config,
      TSDBNodeRef* node);

  void scheduleCompaction();
  void commitState();
  uint64_t replicateTo(const String& addr, uint64_t offset);
  void buildDerivedDataset(RefPtr<DerivedDataset> dset);

  String key_;
  String stream_key_;
  RecordSet records_;
  RefPtr<StreamProperties> config_;
  TSDBNodeRef* node_;
  std::mutex mutex_;
  std::mutex replication_mutex_;
  std::mutex indexbuild_mutex_;
  DateTime last_compaction_;
  HashMap<uint64_t, uint64_t> repl_offsets_;
};

}
}
#endif
