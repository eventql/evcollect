/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2009-2014 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
// or the GNU Lesser General Public License.
/////////////////////////////////////////////////////////////////////////////

#include "fnord-fts/fts.h"
#include "fnord-fts/search/DocIdSet.h"
#include "fnord-fts/search/FilteredDocIdSet.h"
#include "fnord-fts/search/_FilteredDocIdSet.h"

namespace fnord {
namespace fts {

FilteredDocIdSet::FilteredDocIdSet(const DocIdSetPtr& innerSet) {
    this->innerSet = innerSet;
}

FilteredDocIdSet::~FilteredDocIdSet() {
}

bool FilteredDocIdSet::isCacheable() {
    return innerSet->isCacheable();
}

DocIdSetIteratorPtr FilteredDocIdSet::iterator() {
    return newLucene<DefaultFilteredDocIdSetIterator>(shared_from_this(), innerSet->iterator());
}

DefaultFilteredDocIdSetIterator::DefaultFilteredDocIdSetIterator(const FilteredDocIdSetPtr& filtered, const DocIdSetIteratorPtr& innerIter) : FilteredDocIdSetIterator(innerIter) {
    this->filtered = filtered;
}

DefaultFilteredDocIdSetIterator::~DefaultFilteredDocIdSetIterator() {
}

bool DefaultFilteredDocIdSetIterator::match(int32_t docid) {
    return filtered->match(docid);
}

}

}
