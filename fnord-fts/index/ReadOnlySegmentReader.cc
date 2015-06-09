/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2009-2014 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
// or the GNU Lesser General Public License.
/////////////////////////////////////////////////////////////////////////////

#include "fnord-fts/fts.h"
#include "fnord-fts/index/ReadOnlySegmentReader.h"
#include "fnord-fts/util/BitVector.h"

namespace fnord {
namespace fts {

ReadOnlySegmentReader::~ReadOnlySegmentReader() {
}

void ReadOnlySegmentReader::noWrite() {
    boost::throw_exception(UnsupportedOperationException(L"This IndexReader cannot make any changes to the index (it was opened with readOnly = true)"));
}

void ReadOnlySegmentReader::acquireWriteLock() {
    noWrite();
}

bool ReadOnlySegmentReader::isDeleted(int32_t n) {
    return (deletedDocs && deletedDocs->get(n));
}

}

}
