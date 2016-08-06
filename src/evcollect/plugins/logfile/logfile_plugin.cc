/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include <netdb.h>
#include <unistd.h>
#include <list>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <evcollect/util/stringutil.h>
#include "logfile_plugin.h"

namespace evcollect {
namespace plugin_logfile {

class LogfileSource {
public:
  LogfileSource(
      const std::string& filename,
      size_t inode,
      size_t offset);

  bool hasNextLine();
  ReturnCode getNextLine(std::string* line);
  size_t getOffset() const;

protected:
  std::string filename_;
  size_t inode_;
  size_t offset_;
  size_t consumed_offset_;
  char buf_[8192];
  size_t buf_len_;
  size_t buf_pos_;
  std::list<std::string> line_buf_;
  ReturnCode readLines();
  bool readLine(int fd, std::string* line);
  bool readNextByte(int fd, char* target);
};

LogfileSource::LogfileSource(
    const std::string& filename,
    size_t inode,
    size_t offset) :
    filename_(filename),
    inode_(inode),
    offset_(offset),
    consumed_offset_(offset) {}

bool LogfileSource::hasNextLine() {
  if (line_buf_.empty()) {
    readLines();
  }

  return !line_buf_.empty();
}

ReturnCode LogfileSource::getNextLine(std::string* line) {
  if (line_buf_.empty()) {
    auto rc = readLines();
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  if (!line_buf_.empty()) {
    *line = line_buf_.front();
    consumed_offset_ += line_buf_.front().size();
    line_buf_.pop_front();
  }

  return ReturnCode::success();
}

size_t LogfileSource::getOffset() const {
  return consumed_offset_;
}

ReturnCode LogfileSource::readLines() {
  struct stat file_st;
  if (stat(filename_.c_str(), &file_st) < 0) {
    return ReturnCode::error("IOERR", "fstat('%s') failed", filename_.c_str());
  }

  uint64_t file_inode = file_st.st_ino;
  uint64_t file_size = file_st.st_size;
  if (file_inode != inode_ || file_size < offset_) {
    line_buf_.clear();
    inode_ = file_inode;
    offset_ = 0;
    consumed_offset_ = 0;
  }

  int fd = open(filename_.c_str(), O_RDONLY, 0);
  if (fd < 0) {
    return ReturnCode::error("IOERR", "open('%s') failed", filename_.c_str());
  }

  if (lseek(fd, offset_, SEEK_SET) < 0) {
    close(fd);
    return ReturnCode::error("IOERR", "lseek('%i') failed", fd);
  }

  buf_len_ = 0;
  buf_pos_ = 0;

  std::string line;
  while (readLine(fd, &line)) {
    line_buf_.push_back(line);
    offset_ += line.size();
  }

  close(fd);
  return ReturnCode::success();
}

bool LogfileSource::readLine(int fd, std::string* line) {
  line->clear();

  char byte;
  while (readNextByte(fd, &byte)) {
    *line += byte;

    if (byte == '\n') {
      return true;
    }
  }

  return false;
}

bool LogfileSource::readNextByte(int fd, char* target) {
  if (buf_pos_ >= buf_len_) {
    int bytes_read = read(fd, buf_, sizeof(buf_));
    if (bytes_read <= 0) {
      return false; // FIXME?
    }

    buf_pos_ = 0;
    buf_len_ = bytes_read;
  }

  if (buf_pos_ < buf_len_) {
    *target = buf_[buf_pos_++];
    return true;
  } else {
    return false;
  }
}

ReturnCode LogfileSourcePlugin::pluginAttach(
    const PropertyList& config,
    void** userdata) {
  *userdata = new LogfileSource("/tmp/log", 0, 0);
  return ReturnCode::success();
}

void LogfileSourcePlugin::pluginDetach(void* userdata) {
  delete static_cast<LogfileSource*>(userdata);
}

ReturnCode LogfileSourcePlugin::pluginGetNextEvent(
    void* userdata,
    std::string* event_json) {
  std::string raw_line;
  {
    auto rc = static_cast<LogfileSource*>(userdata)->getNextLine(&raw_line);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  if (raw_line.empty()) {
    return ReturnCode::success();
  }

  *event_json = StringUtil::format(
      R"({ "data": "$0" })",
      StringUtil::jsonEscape(raw_line));

  return ReturnCode::success();
}

bool LogfileSourcePlugin::pluginHasPendingEvent(
    void* userdata) {
  return static_cast<LogfileSource*>(userdata)->hasNextLine();
}

} // namespace plugins_logfile
} // namespace evcollect
