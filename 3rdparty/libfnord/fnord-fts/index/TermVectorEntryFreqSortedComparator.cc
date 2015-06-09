/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2009-2014 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
// or the GNU Lesser General Public License.
/////////////////////////////////////////////////////////////////////////////

#include "fnord-fts/fts.h"
#include "fnord-fts/index/TermVectorEntryFreqSortedComparator.h"
#include "fnord-fts/index/TermVectorEntry.h"

namespace fnord {
namespace fts {

TermVectorEntryFreqSortedComparator::~TermVectorEntryFreqSortedComparator() {
}

bool TermVectorEntryFreqSortedComparator::compare(const TermVectorEntryPtr& first, const TermVectorEntryPtr& second) {
    int32_t result = (second->getFrequency() - first->getFrequency());
    if (result < 0) {
        return true;
    }
    if (result > 0) {
        return false;
    }
    result = first->getTerm().compare(second->getTerm());
    if (result < 0) {
        return true;
    }
    if (result > 0) {
        return false;
    }
    return (first->getField().compare(second->getField()) < 0);
}

}

}
