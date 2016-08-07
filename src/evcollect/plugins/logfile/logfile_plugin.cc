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
#include <string.h>
#include <unistd.h>
#include <evcollect/util/stringutil.h>
#include <evcollect/util/time.h>
#include <evcollect/util/logging.h>
#include "logfile_plugin.h"
#include <pcre.h>

namespace evcollect {
namespace plugin_logfile {

class LogfileSource {
public:
  LogfileSource(const std::string& filename);
  ~LogfileSource();

  ReturnCode setRegex(const std::string& regex);

  bool hasNextLine();
  ReturnCode getNextLine(std::string* line);
  ReturnCode getNextEvent(std::string* event_json);

  ReturnCode readCheckpoint();
  ReturnCode writeCheckpoint();

protected:
  std::string filename_;
  std::string checkpoint_filename_;
  pcre* pcre_handle_;
  std::vector<std::string> pcre_fields_;
  uint64_t inode_;
  uint64_t offset_;
  uint64_t consumed_offset_;
  uint64_t checkpoint_inode_;
  uint64_t checkpoint_offset_;
  uint64_t checkpoint_interval_micros_;
  uint64_t last_checkpoint_;
  char buf_[8192];
  size_t buf_len_;
  size_t buf_pos_;
  std::list<std::string> line_buf_;
  uint64_t line_buf_maxsize_;
  ReturnCode readLines();
  bool readLine(int fd, std::string* line);
  bool readNextByte(int fd, char* target);
};

LogfileSource::LogfileSource(
    const std::string& filename) :
    filename_(filename),
    checkpoint_filename_(filename + ".coff"),
    pcre_handle_(nullptr),
    inode_(0),
    offset_(0),
    consumed_offset_(0),
    checkpoint_inode_(0),
    checkpoint_offset_(0),
    checkpoint_interval_micros_(10 * kMicrosPerSecond),
    last_checkpoint_(0),
    line_buf_maxsize_(8192) {}

LogfileSource::~LogfileSource() {
  if (pcre_handle_) {
    pcre_free(pcre_handle_);
  }
}

ReturnCode LogfileSource::setRegex(const std::string& regex) {
  const char* error_msg = "";
  int error_pos = 0;

  pcre_handle_ = pcre_compile(
      regex.c_str(),
      0,
      &error_msg,
      &error_pos,
      0);

  if (!pcre_handle_) {
    return ReturnCode::error("REGEX_ERROR", "invalid regex: %s", error_msg);
  }

  int namecount = 0;
  int capture_count = 0;
  pcre_fullinfo(pcre_handle_, NULL, PCRE_INFO_NAMECOUNT, &namecount);
  pcre_fullinfo(pcre_handle_, NULL, PCRE_INFO_CAPTURECOUNT, &capture_count);

  if (namecount < 1) {
    pcre_free(pcre_handle_);
    pcre_handle_ = nullptr;

    return ReturnCode::error(
        "REGEX_ERROR",
        "regex has no named capture groups");
  }

  unsigned char* name_table;
  int name_entry_size;
  pcre_fullinfo(pcre_handle_, NULL, PCRE_INFO_NAMETABLE, &name_table);
  pcre_fullinfo(pcre_handle_, NULL, PCRE_INFO_NAMEENTRYSIZE, &name_entry_size);
  pcre_fields_.clear();
  pcre_fields_.resize(capture_count + 1);
  auto tabptr = name_table;
  for (int i = 0; i < namecount; i++) {
    int idx = (tabptr[0] << 8) | tabptr[1];
    pcre_fields_[idx] = std::string(
        (const char*) tabptr + 2,
        name_entry_size - 3);

    tabptr += name_entry_size;
  }

  return ReturnCode::success();
}

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

  auto now = WallClock::unixMicros();
  if (now - last_checkpoint_ >= checkpoint_interval_micros_) {
    auto rc = writeCheckpoint();
    if (!rc.isSuccess()) {
      logWarning("error while writing checkpoint file: $0", rc.getMessage());
    }
    last_checkpoint_ = WallClock::unixMicros();
  }

  return ReturnCode::success();
}

ReturnCode LogfileSource::getNextEvent(std::string* event_json) {
  std::string raw_line;
  {
    auto rc = getNextLine(&raw_line);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  if (raw_line.empty()) {
    return ReturnCode::success();
  }

  if (pcre_handle_) {
    const size_t OV_COUNT = 3 * 36;
    int ovector[OV_COUNT];

    int pcre_rc = pcre_exec(
        pcre_handle_,
        0,
        raw_line.data(),
        raw_line.size(),
        0,
        0,
        ovector,
        OV_COUNT);

    if (pcre_rc >= 0) {
      *event_json += "{";
      size_t n = 0;
      for (int i = 1; i < pcre_rc; ++i) {
        if (pcre_fields_[i].empty()) {
          continue;
        }

        if (++n > 1) {
          *event_json += ",";
        }

        std::string evdata(
            raw_line.data() + ovector[2*i],
            ovector[2*i+1] - ovector[2*i]);

        *event_json += "\"" + StringUtil::jsonEscape(pcre_fields_[i]) + "\":";
        *event_json += "\"" + StringUtil::jsonEscape(evdata) + "\"";
      }

      *event_json += "}";
    }
  } else {
    *event_json = StringUtil::format(
        R"({ "data": "$0" })",
        StringUtil::jsonEscape(raw_line));
  }

  return ReturnCode::success();
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

  if (file_size == offset_) {
    return ReturnCode::success();
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
  while (readLine(fd, &line) && line_buf_.size() < line_buf_maxsize_) {
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

ReturnCode LogfileSource::readCheckpoint() {
  inode_ = 0;
  offset_ = 0;

  int fd = open(checkpoint_filename_.c_str(), O_RDONLY);
  if (fd > 0) {
    unsigned char cdata[sizeof(uint64_t) * 2];
    if (read(fd, cdata, sizeof(cdata)) == sizeof(cdata)) {
      memcpy(&inode_, &cdata[sizeof(uint64_t) * 0], sizeof(uint64_t));
      memcpy(&offset_, &cdata[sizeof(uint64_t) * 1], sizeof(uint64_t));
    }

    close(fd);
  }

  consumed_offset_ = offset_;
  checkpoint_inode_ = inode_;
  checkpoint_offset_ = offset_;
  return ReturnCode::success();
}

ReturnCode LogfileSource::writeCheckpoint() {
  if (checkpoint_inode_ == inode_ && checkpoint_offset_ == consumed_offset_) {
    return ReturnCode::success();
  }

  checkpoint_inode_ = inode_;
  checkpoint_offset_ = consumed_offset_;

  unsigned char cdata[sizeof(uint64_t) * 2];
  memcpy(&cdata[sizeof(uint64_t) * 0], &checkpoint_inode_, sizeof(uint64_t));
  memcpy(&cdata[sizeof(uint64_t) * 1], &checkpoint_offset_, sizeof(uint64_t));

  int fd = open(
      checkpoint_filename_.c_str(),
      O_WRONLY | O_TRUNC | O_CREAT,
      0666);

  if (fd < 0) {
    return ReturnCode::error(
        "IOERR",
        "open('%s') failed",
        checkpoint_filename_.c_str());
  }

  write(fd, cdata, sizeof(cdata));
  close(fd);

  return ReturnCode::success();
}

ReturnCode LogfileSourcePlugin::pluginAttach(
    const PropertyList& config,
    void** userdata) {
  auto logfile = new LogfileSource("/tmp/log");
  logfile->readCheckpoint();

  std::string regex;
  if (config.get("regex", &regex)) {
    auto rc = logfile->setRegex(regex);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  *userdata = logfile;
  return ReturnCode::success();
}

void LogfileSourcePlugin::pluginDetach(void* userdata) {
  auto logfile = static_cast<LogfileSource*>(userdata);
  logfile->writeCheckpoint();
  delete logfile;
}

ReturnCode LogfileSourcePlugin::pluginGetNextEvent(
    void* userdata,
    std::string* event_json) {
  return static_cast<LogfileSource*>(userdata)->getNextEvent(event_json);
}

bool LogfileSourcePlugin::pluginHasPendingEvent(
    void* userdata) {
  return static_cast<LogfileSource*>(userdata)->hasNextLine();
}

} // namespace plugins_logfile
} // namespace evcollect
