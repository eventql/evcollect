/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-sstable/binaryformat.h>
#include <fnord-sstable/fileheaderreader.h>
#include <fnord-base/exception.h>
#include <fnord-base/fnv.h>

namespace fnord {
namespace sstable {

FileHeaderReader::FileHeaderReader(
    void* buf,
    size_t buf_size) :
    fnord::util::BinaryMessageReader(buf, buf_size) {
  auto magic_bytes = *readUInt32();
  if (magic_bytes != BinaryFormat::kMagicBytes) {
    RAISE(kIllegalStateError, "not a valid sstable");
  }

  auto version = *readUInt16();
  switch (version) {

    case 0x1:
      flags_ = 0;
      break;

    case 0x2:
      flags_ = *readUInt64();
      break;

    default:
      RAISE(kIllegalStateError, "unsupported sstable version");

  }

  body_size_ = *readUInt64();
  userdata_checksum_ = *readUInt32();
  userdata_size_ = *readUInt32();
  userdata_offset_ = pos_;

  /* pre version 0x02 body_size > 0 implied that the table is finalized */
  if (version == 0x01 && body_size_ > 0) {
    flags_ |= (uint64_t) FileHeaderFlags::FINALIZED;
  }
}

bool FileHeaderReader::verify() {
  if (userdata_offset_ + userdata_size_ > size_) {
    return false;
  }

  if (userdata_size_ == 0) {
    return true;
  }

  const void* userdata;
  size_t userdata_size;
  readUserdata(&userdata, &userdata_size);

  FNV<uint32_t> fnv;
  uint32_t userdata_checksum = fnv.hash(userdata, userdata_size);

  return userdata_checksum == userdata_checksum_;
}

size_t FileHeaderReader::headerSize() const {
  return userdata_offset_ + userdata_size_;
}

size_t FileHeaderReader::bodySize() const {
  return body_size_;
}

bool FileHeaderReader::isFinalized() const {
  return (flags_ & (uint64_t) FileHeaderFlags::FINALIZED) > 0;
}

size_t FileHeaderReader::userdataSize() const {
  return userdata_size_;
}

void FileHeaderReader::readUserdata(
    const void** userdata,
    size_t* userdata_size) {
  seekTo(userdata_offset_);
  *userdata = read(userdata_size_);
  *userdata_size = userdata_size_;
}

}
}

