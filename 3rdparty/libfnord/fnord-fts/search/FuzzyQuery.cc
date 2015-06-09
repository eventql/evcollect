/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2009-2014 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
// or the GNU Lesser General Public License.
/////////////////////////////////////////////////////////////////////////////

#include "fnord-fts/fts.h"
#include "fnord-fts/search/FuzzyQuery.h"
#include "fnord-fts/search/_FuzzyQuery.h"
#include "fnord-fts/search/FuzzyTermEnum.h"
#include "fnord-fts/index/Term.h"
#include "fnord-fts/search/TermQuery.h"
#include "fnord-fts/search/BooleanQuery.h"
#include "fnord-fts/search/BooleanClause.h"
#include "fnord-fts/util/MiscUtils.h"

namespace fnord {
namespace fts {

const int32_t FuzzyQuery::defaultPrefixLength = 0;

FuzzyQuery::FuzzyQuery(const TermPtr& term, double minimumSimilarity, int32_t prefixLength) {
    ConstructQuery(term, minimumSimilarity, prefixLength);
}

FuzzyQuery::FuzzyQuery(const TermPtr& term, double minimumSimilarity) {
    ConstructQuery(term, minimumSimilarity, defaultPrefixLength);
}

FuzzyQuery::FuzzyQuery(const TermPtr& term) {
    ConstructQuery(term, defaultMinSimilarity(), defaultPrefixLength);
}

FuzzyQuery::~FuzzyQuery() {
}

void FuzzyQuery::ConstructQuery(const TermPtr& term, double minimumSimilarity, int32_t prefixLength) {
    this->term = term;

    if (minimumSimilarity >= 1.0) {
        boost::throw_exception(IllegalArgumentException(L"minimumSimilarity >= 1"));
    } else if (minimumSimilarity < 0.0) {
        boost::throw_exception(IllegalArgumentException(L"minimumSimilarity < 0"));
    }
    if (prefixLength < 0) {
        boost::throw_exception(IllegalArgumentException(L"prefixLength < 0"));
    }

    this->termLongEnough = ((int32_t)term->text().length() > (int32_t)(1.0 / (1.0 - minimumSimilarity)));

    this->minimumSimilarity = minimumSimilarity;
    this->prefixLength = prefixLength;
    rewriteMethod = SCORING_BOOLEAN_QUERY_REWRITE();
}

double FuzzyQuery::defaultMinSimilarity() {
    const double _defaultMinSimilarity = 0.5;
    return _defaultMinSimilarity;
}

double FuzzyQuery::getMinSimilarity() {
    return minimumSimilarity;
}

int32_t FuzzyQuery::getPrefixLength() {
    return prefixLength;
}

FilteredTermEnumPtr FuzzyQuery::getEnum(const IndexReaderPtr& reader) {
    return newLucene<FuzzyTermEnum>(reader, getTerm(), minimumSimilarity, prefixLength);
}

TermPtr FuzzyQuery::getTerm() {
    return term;
}

void FuzzyQuery::setRewriteMethod(const RewriteMethodPtr& method) {
    boost::throw_exception(UnsupportedOperationException(L"FuzzyQuery cannot change rewrite method"));
}

QueryPtr FuzzyQuery::rewrite(const IndexReaderPtr& reader) {
    if (!termLongEnough) { // can only match if it's exact
        return newLucene<TermQuery>(term);
    }

    int32_t maxSize = BooleanQuery::getMaxClauseCount();
    ScoreTermQueuePtr stQueue(newLucene<ScoreTermQueue>(1024));
    FilteredTermEnumPtr enumerator(getEnum(reader));
    LuceneException finally;
    try {
        ScoreTermPtr st = newLucene<ScoreTerm>();
        do {
            TermPtr t(enumerator->term());
            if (!t) {
                break;
            }
            double score = enumerator->difference();
            // ignore uncompetitive hits
            if (stQueue->size() >= maxSize && score <= stQueue->top()->score) {
                continue;
            }
            // add new entry in PQ
            st->term = t;
            st->score = score;
            stQueue->add(st);
            // possibly drop entries from queue
            st = (stQueue->size() > maxSize) ? stQueue->pop() : newLucene<ScoreTerm>();
        } while (enumerator->next());
    } catch (LuceneException& e) {
        finally = e;
    }
    enumerator->close();
    finally.throwException();

    BooleanQueryPtr query(newLucene<BooleanQuery>(true));
    int32_t size = stQueue->size();
    for (int32_t i = 0; i < size; ++i) {
        ScoreTermPtr st(stQueue->pop());
        TermQueryPtr tq(newLucene<TermQuery>(st->term)); // found a match
        tq->setBoost(getBoost() * st->score); // set the boost
        query->add(tq, BooleanClause::SHOULD); // add to query
    }

    return query;
}

LuceneObjectPtr FuzzyQuery::clone(const LuceneObjectPtr& other) {
    LuceneObjectPtr clone = MultiTermQuery::clone(other ? other : newLucene<FuzzyQuery>(term));
    FuzzyQueryPtr cloneQuery(std::dynamic_pointer_cast<FuzzyQuery>(clone));
    cloneQuery->minimumSimilarity = minimumSimilarity;
    cloneQuery->prefixLength = prefixLength;
    cloneQuery->termLongEnough = termLongEnough;
    cloneQuery->term = term;
    return cloneQuery;
}

String FuzzyQuery::toString(const String& field) {
    StringStream buffer;
    if (term->field() != field) {
        buffer << term->field() << L":";
    }
    buffer << term->text() << L"~" << minimumSimilarity << boostString();
    return buffer.str();
}

int32_t FuzzyQuery::hashCode() {
    int32_t prime = 31;
    int32_t result = MultiTermQuery::hashCode();
    result = prime * result + MiscUtils::doubleToIntBits(minimumSimilarity);
    result = prime * result + prefixLength;
    result = prime * result + (term ? term->hashCode() : 0);
    return result;
}

bool FuzzyQuery::equals(const LuceneObjectPtr& other) {
    if (LuceneObject::equals(other)) {
        return true;
    }
    if (!MultiTermQuery::equals(other)) {
        return false;
    }
    if (!MiscUtils::equalTypes(shared_from_this(), other)) {
        return false;
    }
    FuzzyQueryPtr otherFuzzyQuery(std::dynamic_pointer_cast<FuzzyQuery>(other));
    if (!otherFuzzyQuery) {
        return false;
    }
    if (MiscUtils::doubleToIntBits(minimumSimilarity) != MiscUtils::doubleToIntBits(otherFuzzyQuery->minimumSimilarity)) {
        return false;
    }
    if (prefixLength != otherFuzzyQuery->prefixLength) {
        return false;
    }
    if (!term) {
        if (otherFuzzyQuery->term) {
            return false;
        }
    } else if (!term->equals(otherFuzzyQuery->term)) {
        return false;
    }
    return true;
}

ScoreTerm::~ScoreTerm() {
}

int32_t ScoreTerm::compareTo(const ScoreTermPtr& other) {
    if (this->score == other->score) {
        return other->term->compareTo(this->term);
    } else {
        return this->score < other->score ? -1 : (this->score > other->score ? 1 : 0);
    }
}

ScoreTermQueue::ScoreTermQueue(int32_t size) : PriorityQueue<ScoreTermPtr>(size) {
}

ScoreTermQueue::~ScoreTermQueue() {
}

bool ScoreTermQueue::lessThan(const ScoreTermPtr& first, const ScoreTermPtr& second) {
    return (first->compareTo(second) < 0);
}

}

}
