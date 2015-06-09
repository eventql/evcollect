/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2009-2014 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
// or the GNU Lesser General Public License.
/////////////////////////////////////////////////////////////////////////////

#include "fnord-fts/fts.h"
#include "fnord-fts/search/DocIdSet.h"
#include "fnord-fts/search/_DocIdSet.h"

namespace fnord {
namespace fts {

DocIdSet::~DocIdSet() {
}

bool DocIdSet::isCacheable() {
    return false;
}

DocIdSetPtr DocIdSet::EMPTY_DOCIDSET() {
    static DocIdSetPtr _EMPTY_DOCIDSET;
    if (!_EMPTY_DOCIDSET) {
        _EMPTY_DOCIDSET = newLucene<EmptyDocIdSet>();
        CycleCheck::addStatic(_EMPTY_DOCIDSET);
    }
    return _EMPTY_DOCIDSET;
}

EmptyDocIdSetIterator::~EmptyDocIdSetIterator() {
}

int32_t EmptyDocIdSetIterator::advance(int32_t target) {
    return NO_MORE_DOCS;
}

int32_t EmptyDocIdSetIterator::docID() {
    return NO_MORE_DOCS;
}

int32_t EmptyDocIdSetIterator::nextDoc() {
    return NO_MORE_DOCS;
}

EmptyDocIdSet::~EmptyDocIdSet() {
}

DocIdSetIteratorPtr EmptyDocIdSet::iterator() {
    return newLucene<EmptyDocIdSetIterator>();
}

bool EmptyDocIdSet::isCacheable() {
    return true;
}

}

}
