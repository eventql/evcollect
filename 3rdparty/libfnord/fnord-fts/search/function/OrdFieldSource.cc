/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2009-2014 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
// or the GNU Lesser General Public License.
/////////////////////////////////////////////////////////////////////////////

#include "fnord-fts/fts.h"
#include "fnord-fts/search/function/OrdFieldSource.h"
#include "_OrdFieldSource.h"
#include "fnord-fts/search/FieldCache.h"
#include "fnord-fts/util/MiscUtils.h"
#include "fnord-fts/util/StringUtils.h"

namespace fnord {
namespace fts {

OrdFieldSource::OrdFieldSource(const String& field) {
    this->field = field;
}

OrdFieldSource::~OrdFieldSource() {
}

String OrdFieldSource::description() {
    return L"ord(" + field + L")";
}

DocValuesPtr OrdFieldSource::getValues(const IndexReaderPtr& reader) {
    Collection<int32_t> arr(FieldCache::DEFAULT()->getStringIndex(reader, field)->order);
    return newLucene<OrdDocValues>(shared_from_this(), arr);
}

bool OrdFieldSource::equals(const LuceneObjectPtr& other) {
    if (!MiscUtils::equalTypes(shared_from_this(), other)) {
        return false;
    }
    OrdFieldSourcePtr otherSource(std::dynamic_pointer_cast<OrdFieldSource>(other));
    if (!otherSource) {
        return false;
    }
    return field == otherSource->field;
}

int32_t OrdFieldSource::hashCode() {
    return StringUtils::hashCode(OrdFieldSource::_getClassName()) + StringUtils::hashCode(field);
}

OrdDocValues::OrdDocValues(const OrdFieldSourcePtr& source, Collection<int32_t> arr) {
    this->_source = source;
    this->arr = arr;
}

OrdDocValues::~OrdDocValues() {
}

double OrdDocValues::doubleVal(int32_t doc) {
    if (doc < 0 || doc >= arr.size()) {
        boost::throw_exception(IndexOutOfBoundsException());
    }
    return (double)arr[doc];
}

String OrdDocValues::strVal(int32_t doc) {
    // the string value of the ordinal, not the string itself
    if (doc < 0 || doc >= arr.size()) {
        boost::throw_exception(IndexOutOfBoundsException());
    }
    return StringUtils::toString(arr[doc]);
}

String OrdDocValues::toString(int32_t doc) {
    return OrdFieldSourcePtr(_source)->description() + L"=" + StringUtils::toString(intVal(doc));
}

CollectionValue OrdDocValues::getInnerArray() {
    return arr;
}

}

}
