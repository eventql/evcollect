/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-base/exception.h>
#include <fnord-base/logging.h>
#include <fnord-base/util/binarymessagereader.h>
#include <fnord-base/util/binarymessagewriter.h>
#include <fnord-base/io/FileLock.h>
#include <fnord-base/io/fileutil.h>
#include <fnord-afx/ArtifactIndex.h>

namespace fnord {
namespace logtable {

ArtifactIndex::ArtifactIndex(
    const String& db_path,
    const String& index_name,
    bool readonly) :
    db_path_(db_path),
    index_name_(index_name),
    readonly_(readonly),
    exists_(false),
    index_file_(StringUtil::format("$0/$1.afx", db_path_, index_name_)),
    cached_mtime_(0) {}

List<ArtifactRef> ArtifactIndex::listArtifacts() {
  return readIndex().artifacts;
}

void ArtifactIndex::addArtifact(const ArtifactRef& artifact) {
  fnord::logDebug("fn.evdb", "Adding artifact: $0", artifact.name);

  withIndex(false, [&] (ArtifactIndexSnapshot* index) {
    for (const auto& a : index->artifacts) {
      if (a.name == artifact.name) {
        RAISEF(kIndexError, "artifact '$0' already exists in index", a.name);
      }
    }

    index->artifacts.emplace_back(artifact);
  });
}

void ArtifactIndex::updateStatus(
    const String& artifact_name,
    ArtifactStatus new_status) {
  withIndex(false, [&] (ArtifactIndexSnapshot* index) {
    for (auto& a : index->artifacts) {
      if (a.name == artifact_name) {
        statusTransition(&a, new_status);
        return;
      }
    }

    RAISEF(kIndexError, "artifact '$0' not found", artifact_name);
  });
}

void ArtifactIndex::deleteArtifact(const String& artifact_name) {
  withIndex(false, [&] (ArtifactIndexSnapshot* index) {
    for (auto a = index->artifacts.begin(); a != index->artifacts.end(); ) {
      if (a->name == artifact_name) {
        a = index->artifacts.erase(a);
        return;
      } else {
        ++a;
      }
    }

    RAISEF(kIndexError, "artifact '$0' not found", artifact_name);
  });
}

void ArtifactIndex::statusTransition(
    ArtifactRef* artifact,
    ArtifactStatus new_status) {
  artifact->status = new_status; // FIXPAUL proper transition
}

void ArtifactIndex::withIndex(
    bool readonly,
    Function<void (ArtifactIndexSnapshot* index)> fn) {
  std::unique_lock<std::mutex> lk(mutex_, std::defer_lock);
  FileLock file_lk(index_file_ + ".lck");

  if (!readonly) {
    lk.lock();
    file_lk.lock(true);
  }

  auto index = readIndex();
  fn(&index);

  if (!readonly) {
    writeIndex(index);
  }
}

ArtifactIndexSnapshot ArtifactIndex::readIndex() {
  ArtifactIndexSnapshot index;
  if (!(exists_.load() || FileUtil::exists(index_file_))) {
    return index;
  }

  exists_ = true;

  std::unique_lock<std::mutex> lk(cached_mutex_);
  auto mtime = FileUtil::mtime(index_file_);

  if (mtime == cached_mtime_) {
    return cached_;
  }

  auto file = FileUtil::read(index_file_);
  index.decode(file);

  if (mtime > cached_mtime_) {
    cached_mtime_ = mtime;
    cached_ = index;
  }

  return index;
}

void ArtifactIndex::writeIndex(const ArtifactIndexSnapshot& index) {
  Buffer buf;
  index.encode(&buf);

  auto file = File::openFile(
      index_file_ + "~",
      File::O_CREATEOROPEN | File::O_WRITE | File::O_TRUNCATE);

  file.write(buf.data(), buf.size());
  FileUtil::mv(index_file_ + "~", index_file_);

  std::unique_lock<std::mutex> lk(cached_mutex_);
  cached_mtime_ = FileUtil::mtime(index_file_);
  cached_ = index;
}

const String& ArtifactIndex::basePath() const {
  return db_path_;
}

const String& ArtifactIndex::indexName() const {
  return index_name_;
}

void ArtifactIndex::runConsistencyCheck(
    bool check_checksums /* = false */,
    bool repair /* = false */) {
  bool fail = false;
  auto artifacts = listArtifacts();

  for (const auto& a : artifacts) {
    if (a.status != ArtifactStatus::PRESENT) {
      continue;
    }

    for (const auto& f : a.files) {
      if (f.size == 0) {
        continue;
      }

      auto file_path = FileUtil::joinPaths(db_path_, f.filename);
      if (!FileUtil::exists(file_path)) {
        fnord::logError(
            "fnord.evdb",
            "consistency error: file '$0' from artifact '$1' is marked as " \
            "PRESENT in index but is missing on disk",
            f.filename,
            a.name);

        if (repair) {
          fnord::logError(
              "fnord.evdb",
              "repairing consistency error: in artifact '$0' by marking it " \
              "MISSING",
              a.name);

          updateStatus(a.name, ArtifactStatus::MISSING);
        } else {
          fail = true;
        }
        continue;
      }

      auto file_size = FileUtil::size(file_path);
      if (file_size != f.size) {
        fnord::logError(
            "fnord.evdb",
            "consistency error: file '$0' from artifact '$1' has the wrong" \
            "size; index=$2 disk=$3",
            a.name,
            f.filename,
            f.size,
            file_size);

        fail = true;
        continue;
      }

      if (!check_checksums) {
        continue;
      }

      fnord::logInfo("fnord.evdb", "fsck: checking file: $0", f.filename);
      auto checksum = FileUtil::checksum(file_path);

      if (checksum != f.checksum) {
        fnord::logError(
            "fnord.evdb",
            "consistency error: checksum mismatch for file '$0' from " \
            "artifact '$1'",
            a.name,
            f.filename);

        fail = true;
      }
    }
  }

  if (fail) {
    RAISE(kRuntimeError, "consistency check failed");
  }
}

size_t ArtifactRef::totalSize() const {
  size_t total = 0;

  for (const auto& f : files) {
    total += f.size;
  }

  return total;
}

void ArtifactIndexSnapshot::encode(Buffer* buf) const {
  util::BinaryMessageWriter writer;
  writer.appendUInt8(0x01);
  writer.appendVarUInt(artifacts.size());

  for (const auto& a : artifacts) {
    writer.appendVarUInt(a.name.size());
    writer.append(a.name.data(), a.name.size());
    writer.appendVarUInt((uint8_t) a.status);

    writer.appendVarUInt(a.attributes.size());
    for (const auto& attr : a.attributes) {
      writer.appendVarUInt(attr.first.size());
      writer.append(attr.first.data(), attr.first.size());
      writer.appendVarUInt(attr.second.size());
      writer.append(attr.second.data(), attr.second.size());
    }

    writer.appendVarUInt(a.files.size());
    for (const auto& f : a.files) {
      writer.appendVarUInt(f.filename.size());
      writer.append(f.filename.data(), f.filename.size());
      writer.appendUInt64(f.checksum);
      writer.appendVarUInt(f.size);
    }
  }

  buf->append(writer.data(), writer.size());
}

void ArtifactIndexSnapshot::decode(const Buffer& buf) {
  util::BinaryMessageReader reader(buf.data(), buf.size());
  reader.readUInt8();
  auto num_artifacts = reader.readVarUInt();

  for (int j = 0; j < num_artifacts; ++j) {
    ArtifactRef artifact;

    auto name_len = reader.readVarUInt();
    artifact.name = String((const char*) reader.read(name_len), name_len);
    artifact.status = (ArtifactStatus) ((uint8_t) reader.readVarUInt());

    auto num_attrs = reader.readVarUInt();
    for (int i = 0; i < num_attrs; ++i) {
      auto key_len = reader.readVarUInt();
      String key((const char*) reader.read(key_len), key_len);
      auto value_len = reader.readVarUInt();
      String value((const char*) reader.read(value_len), value_len);
      artifact.attributes.emplace_back(key, value);
    }

    auto num_files = reader.readVarUInt();
    for (int i = 0; i < num_files; ++i) {
      ArtifactFileRef file;
      auto fname_len = reader.readVarUInt();
      file.filename = String((const char*) reader.read(fname_len), fname_len);
      file.checksum = *reader.readUInt64();
      file.size = reader.readVarUInt();
      artifact.files.emplace_back(file);
    }

    artifacts.emplace_back(artifact);
  }
}

} // namespace logtable
} // namespace fnord

