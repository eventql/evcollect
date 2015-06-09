/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2009-2014 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
// or the GNU Lesser General Public License.
/////////////////////////////////////////////////////////////////////////////

#include "fnord-fts/fts.h"
#include "fnord-fts/document/SetBasedFieldSelector.h"

namespace fnord {
namespace fts {

SetBasedFieldSelector::SetBasedFieldSelector(HashSet<String> fieldsToLoad, HashSet<String> lazyFieldsToLoad) {
    this->fieldsToLoad = fieldsToLoad;
    this->lazyFieldsToLoad = lazyFieldsToLoad;
}

SetBasedFieldSelector::~SetBasedFieldSelector() {
}

FieldSelector::FieldSelectorResult SetBasedFieldSelector::accept(const String& fieldName) {
    FieldSelector::FieldSelectorResult result = FieldSelector::SELECTOR_NO_LOAD;
    if (fieldsToLoad.contains(fieldName)) {
        result = FieldSelector::SELECTOR_LOAD;
    }
    if (lazyFieldsToLoad.contains(fieldName)) {
        result = FieldSelector::SELECTOR_LAZY_LOAD;
    }
    return result;
}

}

}
