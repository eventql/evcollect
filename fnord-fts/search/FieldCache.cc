/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2009-2014 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
// or the GNU Lesser General Public License.
/////////////////////////////////////////////////////////////////////////////

#include "fnord-fts/fts.h"
#include "fnord-fts/search/FieldCache.h"
#include "fnord-fts/search/_FieldCache.h"
#include "fnord-fts/search/FieldCacheImpl.h"
#include "fnord-fts/util/NumericUtils.h"
#include "fnord-fts/util/StringUtils.h"

namespace fnord {
namespace fts {

/// Indicator for StringIndex values in the cache.
const int32_t FieldCache::STRING_INDEX = -1;

FieldCache::~FieldCache() {
}

FieldCachePtr FieldCache::DEFAULT() {
    static FieldCacheImplPtr _DEFAULT;
    if (!_DEFAULT) {
        _DEFAULT = newLucene<FieldCacheImpl>();
        CycleCheck::addStatic(_DEFAULT);
    }
    return _DEFAULT;
}

ByteParserPtr FieldCache::DEFAULT_BYTE_PARSER() {
    static DefaultByteParserPtr _DEFAULT_BYTE_PARSER;
    if (!_DEFAULT_BYTE_PARSER) {
        _DEFAULT_BYTE_PARSER = newLucene<DefaultByteParser>();
        CycleCheck::addStatic(_DEFAULT_BYTE_PARSER);
    }
    return _DEFAULT_BYTE_PARSER;
}

IntParserPtr FieldCache::DEFAULT_INT_PARSER() {
    static DefaultIntParserPtr _DEFAULT_INT_PARSER;
    if (!_DEFAULT_INT_PARSER) {
        _DEFAULT_INT_PARSER = newLucene<DefaultIntParser>();
        CycleCheck::addStatic(_DEFAULT_INT_PARSER);
    }
    return _DEFAULT_INT_PARSER;
}

LongParserPtr FieldCache::DEFAULT_LONG_PARSER() {
    static DefaultLongParserPtr _DEFAULT_LONG_PARSER;
    if (!_DEFAULT_LONG_PARSER) {
        _DEFAULT_LONG_PARSER = newLucene<DefaultLongParser>();
        CycleCheck::addStatic(_DEFAULT_LONG_PARSER);
    }
    return _DEFAULT_LONG_PARSER;
}

DoubleParserPtr FieldCache::DEFAULT_DOUBLE_PARSER() {
    static DefaultDoubleParserPtr _DEFAULT_DOUBLE_PARSER;
    if (!_DEFAULT_DOUBLE_PARSER) {
        _DEFAULT_DOUBLE_PARSER = newLucene<DefaultDoubleParser>();
        CycleCheck::addStatic(_DEFAULT_DOUBLE_PARSER);
    }
    return _DEFAULT_DOUBLE_PARSER;
}

IntParserPtr FieldCache::NUMERIC_UTILS_INT_PARSER() {
    static NumericUtilsIntParserPtr _NUMERIC_UTILS_INT_PARSER;
    if (!_NUMERIC_UTILS_INT_PARSER) {
        _NUMERIC_UTILS_INT_PARSER = newLucene<NumericUtilsIntParser>();
        CycleCheck::addStatic(_NUMERIC_UTILS_INT_PARSER);
    }
    return _NUMERIC_UTILS_INT_PARSER;
}

LongParserPtr FieldCache::NUMERIC_UTILS_LONG_PARSER() {
    static NumericUtilsLongParserPtr _NUMERIC_UTILS_LONG_PARSER;
    if (!_NUMERIC_UTILS_LONG_PARSER) {
        _NUMERIC_UTILS_LONG_PARSER = newLucene<NumericUtilsLongParser>();
        CycleCheck::addStatic(_NUMERIC_UTILS_LONG_PARSER);
    }
    return _NUMERIC_UTILS_LONG_PARSER;
}

DoubleParserPtr FieldCache::NUMERIC_UTILS_DOUBLE_PARSER() {
    static NumericUtilsDoubleParserPtr _NUMERIC_UTILS_DOUBLE_PARSER;
    if (!_NUMERIC_UTILS_DOUBLE_PARSER) {
        _NUMERIC_UTILS_DOUBLE_PARSER = newLucene<NumericUtilsDoubleParser>();
        CycleCheck::addStatic(_NUMERIC_UTILS_DOUBLE_PARSER);
    }
    return _NUMERIC_UTILS_DOUBLE_PARSER;
}

Collection<uint8_t> FieldCache::getBytes(const IndexReaderPtr& reader, const String& field) {
    BOOST_ASSERT(false);
    return Collection<uint8_t>(); // override
}

Collection<uint8_t> FieldCache::getBytes(const IndexReaderPtr& reader, const String& field, const ByteParserPtr& parser) {
    BOOST_ASSERT(false);
    return Collection<uint8_t>(); // override
}

Collection<int32_t> FieldCache::getInts(const IndexReaderPtr& reader, const String& field) {
    BOOST_ASSERT(false);
    return Collection<int32_t>(); // override
}

Collection<int32_t> FieldCache::getInts(const IndexReaderPtr& reader, const String& field, const IntParserPtr& parser) {
    BOOST_ASSERT(false);
    return Collection<int32_t>(); // override
}

Collection<int64_t> FieldCache::getLongs(const IndexReaderPtr& reader, const String& field) {
    BOOST_ASSERT(false);
    return Collection<int64_t>(); // override
}

Collection<int64_t> FieldCache::getLongs(const IndexReaderPtr& reader, const String& field, const LongParserPtr& parser) {
    BOOST_ASSERT(false);
    return Collection<int64_t>(); // override
}

Collection<double> FieldCache::getDoubles(const IndexReaderPtr& reader, const String& field) {
    BOOST_ASSERT(false);
    return Collection<double>(); // override
}

Collection<double> FieldCache::getDoubles(const IndexReaderPtr& reader, const String& field, const DoubleParserPtr& parser) {
    BOOST_ASSERT(false);
    return Collection<double>(); // override
}

Collection<String> FieldCache::getStrings(const IndexReaderPtr& reader, const String& field) {
    BOOST_ASSERT(false);
    return Collection<String>(); // override
}

StringIndexPtr FieldCache::getStringIndex(const IndexReaderPtr& reader, const String& field) {
    BOOST_ASSERT(false);
    return StringIndexPtr(); // override
}

void FieldCache::setInfoStream(const InfoStreamPtr& stream) {
    BOOST_ASSERT(false);
    // override
}

InfoStreamPtr FieldCache::getInfoStream() {
    BOOST_ASSERT(false);
    return InfoStreamPtr(); // override
}

CreationPlaceholder::~CreationPlaceholder() {
}

StringIndex::StringIndex(Collection<int32_t> values, Collection<String> lookup) {
    this->order = values;
    this->lookup = lookup;
}

StringIndex::~StringIndex() {
}

int32_t StringIndex::binarySearchLookup(const String& key) {
    Collection<String>::iterator search = std::lower_bound(lookup.begin(), lookup.end(), key);
    int32_t keyPos = std::distance(lookup.begin(), search);
    return (search == lookup.end() || key < *search) ? -(keyPos + 1) : keyPos;
}

Parser::~Parser() {
}

ByteParser::~ByteParser() {
}

uint8_t ByteParser::parseByte(const String& string) {
    return 0; // override
}

DefaultByteParser::~DefaultByteParser() {
}

uint8_t DefaultByteParser::parseByte(const String& string) {
    return (uint8_t)StringUtils::toInt(string);
}

String DefaultByteParser::toString() {
    return FieldCache::_getClassName() + L".DEFAULT_BYTE_PARSER";
}

IntParser::~IntParser() {
}

int32_t IntParser::parseInt(const String& string) {
    return 0; // override
}

DefaultIntParser::~DefaultIntParser() {
}

int32_t DefaultIntParser::parseInt(const String& string) {
    return StringUtils::toInt(string);
}

String DefaultIntParser::toString() {
    return FieldCache::_getClassName() + L".DEFAULT_INT_PARSER";
}

NumericUtilsIntParser::~NumericUtilsIntParser() {
}

int32_t NumericUtilsIntParser::parseInt(const String& string) {
    int32_t shift = string[0] - NumericUtils::SHIFT_START_INT;
    if (shift > 0 && shift <= 31) {
        boost::throw_exception(StopFillCacheException());
    }
    return NumericUtils::prefixCodedToInt(string);
}

String NumericUtilsIntParser::toString() {
    return FieldCache::_getClassName() + L".NUMERIC_UTILS_INT_PARSER";
}

LongParser::~LongParser() {
}

int64_t LongParser::parseLong(const String& string) {
    return 0; // override
}

DefaultLongParser::~DefaultLongParser() {
}

int64_t DefaultLongParser::parseLong(const String& string) {
    return StringUtils::toLong(string);
}

String DefaultLongParser::toString() {
    return FieldCache::_getClassName() + L".DEFAULT_LONG_PARSER";
}

NumericUtilsLongParser::~NumericUtilsLongParser() {
}

int64_t NumericUtilsLongParser::parseLong(const String& string) {
    int32_t shift = string[0] - NumericUtils::SHIFT_START_LONG;
    if (shift > 0 && shift <= 63) {
        boost::throw_exception(StopFillCacheException());
    }
    return NumericUtils::prefixCodedToLong(string);
}

String NumericUtilsLongParser::toString() {
    return FieldCache::_getClassName() + L".NUMERIC_UTILS_LONG_PARSER";
}

DoubleParser::~DoubleParser() {
}

double DoubleParser::parseDouble(const String& string) {
    return 0; // override
}

DefaultDoubleParser::~DefaultDoubleParser() {
}

double DefaultDoubleParser::parseDouble(const String& string) {
    return StringUtils::toDouble(string);
}

String DefaultDoubleParser::toString() {
    return FieldCache::_getClassName() + L".DEFAULT_DOUBLE_PARSER";
}

NumericUtilsDoubleParser::~NumericUtilsDoubleParser() {
}

double NumericUtilsDoubleParser::parseDouble(const String& string) {
    int32_t shift = string[0] - NumericUtils::SHIFT_START_LONG;
    if (shift > 0 && shift <= 63) {
        boost::throw_exception(StopFillCacheException());
    }
    return NumericUtils::sortableLongToDouble(NumericUtils::prefixCodedToLong(string));
}

String NumericUtilsDoubleParser::toString() {
    return FieldCache::_getClassName() + L".NUMERIC_UTILS_DOUBLE_PARSER";
}

FieldCacheEntry::~FieldCacheEntry() {
}

String FieldCacheEntry::toString() {
    StringStream buffer;
    buffer << L"'" << getReaderKey()->toString() << L"'=>" << getFieldName() << L"'," << getCacheType();
    return buffer.str();
}

}

}
