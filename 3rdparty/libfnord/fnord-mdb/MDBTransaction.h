/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_MDBTRANSACTION_H
#define _FNORD_MDBTRANSACTION_H
#include <memory>
#include <vector>
#include <liblmdb/lmdb.h>
#include "fnord-base/autoref.h"
#include "fnord-base/logging.h"
#include "fnord-base/option.h"
#include "fnord-mdb/MDBCursor.h"

namespace fnord {
namespace mdb {

class MDBTransaction : public RefCounted {
public:

  MDBTransaction(MDB_txn* mdb_txn, MDB_dbi mdb_handle);
  ~MDBTransaction();
  MDBTransaction(const MDBTransaction& other) = delete;
  MDBTransaction& operator=(const MDBTransaction& other) = delete;

  RefPtr<MDBCursor> getCursor();

  void commit();
  void abort();
  void autoAbort();

  Option<Buffer> get(const Buffer& key);
  Option<Buffer> get(const String& key);
  bool get(
      const void* key,
      size_t key_size,
      void** value,
      size_t* value_size);

  void insert(const String& key, const String& value);
  void insert(const String& key, const Buffer& value);
  void insert(const String& key, const void* value, size_t value_size);
  void insert(const Buffer& key, const Buffer& value);
  void insert(const Buffer& key, const void* value, size_t value_size);
  void insert(
      const void* key,
      size_t key_size,
      const void* value,
      size_t value_size);

  void update(const String& key, const String& value);
  void update(const String& key, const Buffer& value);
  void update(const String& key, const void* value, size_t value_size);
  void update(const Buffer& key, const Buffer& value);
  void update(const Buffer& key, const void* value, size_t value_size);
  void update(
      const void* key,
      size_t key_size,
      const void* value,
      size_t value_size);

  void del(const String& key);
  void del(const Buffer& key);
  void del(const void* key, size_t key_size);

protected:
  MDB_txn* mdb_txn_;
  MDB_dbi mdb_handle_;
  bool is_commited_;
  bool abort_on_free_;
};

}
}
#endif
