/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-cstable/BooleanColumnReader.h>

namespace fnord {
namespace cstable {

BooleanColumnReader::BooleanColumnReader(
    uint64_t r_max,
    uint64_t d_max,
    void* data,
    size_t size) :
    ColumnReader(r_max, d_max, data, size),
    data_reader_(data_, data_size_, 1) {}

bool BooleanColumnReader::next(
    uint64_t* rep_level,
    uint64_t* def_level,
    void** data,
    size_t* data_len) {
  bool val;
  if (next(rep_level, def_level, &val)) {
    cur_val_ = val ? 1 : 0;

    *data = &cur_val_;
    *data_len = sizeof(cur_val_);
    return true;
  } else {
    *data = nullptr;
    *data_len = 0;
    return false;
  }
}

bool BooleanColumnReader::next(
    uint64_t* rep_level,
    uint64_t* def_level,
    bool* data) {
  auto r = rlvl_reader_.next();
  auto d = dlvl_reader_.next();

  *rep_level = r;
  *def_level = d;
  ++vals_read_;

  if (d == d_max_) {
    *data = data_reader_.next();
    return true;
  } else {
    *data = false;
    return false;
  }
}

} // namespace cstable
} // namespace fnord
