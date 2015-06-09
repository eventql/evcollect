/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2009-2014 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
// or the GNU Lesser General Public License.
/////////////////////////////////////////////////////////////////////////////

#include "fnord-fts/fts.h"
#include "fnord-fts/search/SortField.h"
#include "fnord-fts/search/FieldCache.h"
#include "fnord-fts/search/FieldComparator.h"
#include "fnord-fts/search/FieldComparatorSource.h"
#include "fnord-fts/util/StringUtils.h"

namespace fnord {
namespace fts {

/// Sort by document score (relevancy).  Sort values are Double and higher values are at the front.
const int32_t SortField::SCORE = 0;

/// Sort by document number (index order).  Sort values are Integer and lower values are at the front.
const int32_t SortField::DOC = 1;

/// Sort using term values as Strings.  Sort values are String and lower values are at the front.
const int32_t SortField::STRING = 3;

/// Sort using term values as Integers.  Sort values are Integer and lower values are at the front.
const int32_t SortField::INT = 4;

/// Sort using term values as Floats.  Sort values are Float and lower values are at the front.
const int32_t SortField::FLOAT = 5;

/// Sort using term values as Longs.  Sort values are Long and lower values are at the front.
const int32_t SortField::LONG = 6;

/// Sort using term values as Doubles.  Sort values are Double and lower values are at the front.
const int32_t SortField::DOUBLE = 7;

/// Sort using term values as Shorts.  Sort values are Short and lower values are at the front.
const int32_t SortField::SHORT = 8;

/// Sort using a custom Comparator.  Sort values are any ComparableValue and sorting is done according
/// to natural order.
const int32_t SortField::CUSTOM = 9;

/// Sort using term values as Bytes.  Sort values are Byte and lower values are at the front.
const int32_t SortField::BYTE = 10;

/// Sort using term values as Strings, but comparing by value (using String::compare) for all comparisons.
/// This is typically slower than {@link #STRING}, which uses ordinals to do the sorting.
const int32_t SortField::STRING_VAL = 11;

SortField::SortField(const String& field, int32_t type, bool reverse) {
    initFieldType(field, type);
    this->reverse = reverse;
}

SortField::SortField(const String& field, const ParserPtr& parser, bool reverse) {
    if (std::dynamic_pointer_cast<IntParser>(parser)) {
        initFieldType(field, INT);
    } else if (std::dynamic_pointer_cast<ByteParser>(parser)) {
        initFieldType(field, BYTE);
    } else if (std::dynamic_pointer_cast<LongParser>(parser)) {
        initFieldType(field, LONG);
    } else if (std::dynamic_pointer_cast<DoubleParser>(parser)) {
        initFieldType(field, DOUBLE);
    } else {
        boost::throw_exception(IllegalArgumentException(L"Parser instance does not subclass existing numeric parser from FieldCache"));
    }
    this->reverse = reverse;
    this->parser = parser;
}

SortField::SortField(const String& field, const std::locale& locale, bool reverse) {
    initFieldType(field, STRING);
    this->locale = newInstance<std::locale>(locale);
    this->reverse = reverse;
}

SortField::SortField(const String& field, const FieldComparatorSourcePtr& comparator, bool reverse) {
    initFieldType(field, CUSTOM);
    this->comparatorSource = comparator;
    this->reverse = reverse;
}

SortField::~SortField() {
}

SortFieldPtr SortField::FIELD_SCORE() {
    static SortFieldPtr _FIELD_SCORE;
    if (!_FIELD_SCORE) {
        _FIELD_SCORE = newLucene<SortField>(L"", SCORE);
        CycleCheck::addStatic(_FIELD_SCORE);
    }
    return _FIELD_SCORE;
}

SortFieldPtr SortField::FIELD_DOC() {
    static SortFieldPtr _FIELD_DOC;
    if (!_FIELD_DOC) {
        _FIELD_DOC = newLucene<SortField>(L"", DOC);
        CycleCheck::addStatic(_FIELD_DOC);
    }
    return _FIELD_DOC;
}

void SortField::initFieldType(const String& field, int32_t type) {
    this->type = type;
    if (field.empty() && type != SCORE && type != DOC) {
        boost::throw_exception(IllegalArgumentException(L"Field can only be null when type is SCORE or DOC"));
    }
    this->field = field;
}

String SortField::getField() {
    return field;
}

int32_t SortField::getType() {
    return type;
}

localePtr SortField::getLocale() {
    return locale;
}

ParserPtr SortField::getParser() {
    return parser;
}

bool SortField::getReverse() {
    return reverse;
}

FieldComparatorSourcePtr SortField::getComparatorSource() {
    return comparatorSource;
}

String SortField::toString() {
    StringStream buffer;
    switch (type) {
    case SCORE:
        buffer << L"<score>";
        break;
    case DOC:
        buffer << L"<doc>";
        break;
    case STRING:
        buffer << L"<string: \"" << field << L"\">";
        break;
    case STRING_VAL:
        buffer << L"<string_val: \"" << field << L"\">";
        break;
    case BYTE:
        buffer << L"<byte: \"" << field << L"\">";
        break;
    case SHORT:
        buffer << L"<short: \"" << field << L"\">";
        break;
    case INT:
        buffer << L"<int: \"" << field << L"\">";
        break;
    case LONG:
        buffer << L"<long: \"" << field << L"\">";
        break;
    case FLOAT:
        buffer << L"<float: \"" << field << L"\">";
        break;
    case DOUBLE:
        buffer << L"<double: \"" << field << L"\">";
        break;
    case CUSTOM:
        buffer << L"<custom: \"" << field << L"\": " << comparatorSource->toString() << L">";
        break;
    default:
        buffer << L"<???: \"" << field << L"\">";
        break;
    }

    if (parser) {
        buffer << L"(" << parser->toString() << L")";
    }
    if (reverse) {
        buffer << L"!";
    }

    return buffer.str();
}

bool SortField::equals(const LuceneObjectPtr& other) {
    if (LuceneObject::equals(other)) {
        return true;
    }

    SortFieldPtr otherSortField(std::dynamic_pointer_cast<SortField>(other));
    if (!otherSortField) {
        return false;
    }

    return (field == otherSortField->field && type == otherSortField->type &&
            reverse == otherSortField->reverse &&
            ((locale && otherSortField->locale && *locale == *otherSortField->locale) || (!locale && !otherSortField->locale)) &&
            (comparatorSource ? comparatorSource->equals(otherSortField->comparatorSource) : !otherSortField->comparatorSource) &&
            (parser ? parser->equals(otherSortField->parser) : !otherSortField->parser));
}

int32_t SortField::hashCode() {
    int32_t hash = type ^ 0x346565dd + (reverse ? 1 : 0) ^ 0xaf5998bb;
    hash += StringUtils::hashCode(field) ^ 0xff5685dd;
    if (locale) {
        hash += StringUtils::hashCode(StringUtils::toUnicode(locale->name().c_str())) ^ 0xff5685dd;
    }
    if (comparatorSource) {
        hash += comparatorSource->hashCode();
    }
    if (parser) {
        hash += parser->hashCode() ^ 0x3aaf56ff;
    }
    return hash;
}

FieldComparatorPtr SortField::getComparator(int32_t numHits, int32_t sortPos) {
    if (locale) {
        return newLucene<StringComparatorLocale>(numHits, field, *locale);
    }

    switch (type) {
    case SCORE:
        return newLucene<RelevanceComparator>(numHits);
    case DOC:
        return newLucene<DocComparator>(numHits);
    case SHORT:
    case INT:
        return newLucene<IntComparator>(numHits, field, parser);
    case FLOAT:
    case DOUBLE:
        return newLucene<DoubleComparator>(numHits, field, parser);
    case LONG:
        return newLucene<LongComparator>(numHits, field, parser);
    case BYTE:
        return newLucene<ByteComparator>(numHits, field, parser);
    case CUSTOM:
        BOOST_ASSERT(comparatorSource);
        return comparatorSource->newComparator(field, numHits, sortPos, reverse);
    case STRING:
        return newLucene<StringOrdValComparator>(numHits, field, sortPos, reverse);
    case STRING_VAL:
        return newLucene<StringValComparator>(numHits, field);
    default:
        boost::throw_exception(IllegalStateException(L"Illegal sort type: " + StringUtils::toString(type)));
        return FieldComparatorPtr();
    }
}

}

}
