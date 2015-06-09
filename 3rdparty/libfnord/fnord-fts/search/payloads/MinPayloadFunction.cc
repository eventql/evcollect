/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2009-2014 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
// or the GNU Lesser General Public License.
/////////////////////////////////////////////////////////////////////////////

#include "fnord-fts/fts.h"
#include "fnord-fts/search/payloads/MinPayloadFunction.h"
#include "fnord-fts/util/MiscUtils.h"
#include "fnord-fts/util/StringUtils.h"

namespace fnord {
namespace fts {

MinPayloadFunction::~MinPayloadFunction() {
}

double MinPayloadFunction::currentScore(int32_t docId, const String& field, int32_t start, int32_t end,
                                        int32_t numPayloadsSeen, double currentScore, double currentPayloadScore) {
    if (numPayloadsSeen == 0) {
        return currentPayloadScore;
    } else {
        return std::min(currentPayloadScore, currentScore);
    }
}

double MinPayloadFunction::docScore(int32_t docId, const String& field, int32_t numPayloadsSeen, double payloadScore) {
    return numPayloadsSeen > 0 ? payloadScore : 1.0;
}

int32_t MinPayloadFunction::hashCode() {
    int32_t prime = 31;
    int32_t result = 1;
    result = prime * result + StringUtils::hashCode(getClassName());
    return result;
}

bool MinPayloadFunction::equals(const LuceneObjectPtr& other) {
    if (LuceneObject::equals(other)) {
        return true;
    }
    if (!other) {
        return false;
    }
    if (!MiscUtils::equalTypes(shared_from_this(), other)) {
        return false;
    }
    return true;
}

}

}
