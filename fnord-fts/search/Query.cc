/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2009-2014 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
// or the GNU Lesser General Public License.
/////////////////////////////////////////////////////////////////////////////

#include "fnord-fts/fts.h"
#include "fnord-fts/search/Query.h"
#include "fnord-fts/search/BooleanQuery.h"
#include "fnord-fts/search/Searcher.h"
#include "fnord-fts/search/Similarity.h"
#include "fnord-fts/util/MiscUtils.h"

namespace fnord {
namespace fts {

Query::Query() {
    boost = 1.0;
}

Query::~Query() {
}

void Query::setBoost(double boost) {
    this->boost = boost;
}

double Query::getBoost() {
    return boost;
}

String Query::toString(const String& field) {
    return L""; // override
}

String Query::toString() {
    return toString(L"");
}

WeightPtr Query::createWeight(const SearcherPtr& searcher) {
    boost::throw_exception(UnsupportedOperationException());
    return WeightPtr();
}

WeightPtr Query::weight(const SearcherPtr& searcher) {
    QueryPtr query(searcher->rewrite(shared_from_this()));
    WeightPtr weight(query->createWeight(searcher));
    double sum = weight->sumOfSquaredWeights();
    double norm = getSimilarity(searcher)->queryNorm(sum);
    if (MiscUtils::isInfinite(norm) || MiscUtils::isNaN(norm)) {
        norm = 1.0;
    }
    weight->normalize(norm);
    return weight;
}

QueryPtr Query::rewrite(const IndexReaderPtr& reader) {
    return shared_from_this();
}

QueryPtr Query::combine(Collection<QueryPtr> queries) {
    SetQuery uniques(SetQuery::newInstance());
    for (Collection<QueryPtr>::iterator query = queries.begin(); query != queries.end(); ++query) {
        Collection<BooleanClausePtr> clauses;
        BooleanQueryPtr bq(std::dynamic_pointer_cast<BooleanQuery>(*query));
        // check if we can split the query into clauses
        bool splittable = bq.get() != NULL;
        if (splittable) {
            splittable = bq->isCoordDisabled();
            clauses = bq->getClauses();
            for (Collection<BooleanClausePtr>::iterator clause = clauses.begin(); splittable && clause != clauses.end(); ++clause) {
                splittable = ((*clause)->getOccur() == BooleanClause::SHOULD);
            }
        }
        if (splittable) {
            for (Collection<BooleanClausePtr>::iterator clause = clauses.begin(); clause != clauses.end(); ++clause) {
                uniques.add((*clause)->getQuery());
            }
        } else {
            uniques.add(*query);
        }
    }
    // optimization: if we have just one query, just return it
    if (uniques.size() == 1) {
        return *uniques.begin();
    }
    BooleanQueryPtr result(newLucene<BooleanQuery>(true));
    for (SetQuery::iterator query = uniques.begin(); query != uniques.end(); ++query) {
        result->add(*query, BooleanClause::SHOULD);
    }
    return result;
}

void Query::extractTerms(SetTerm terms) {
    // needs to be implemented by query subclasses
    boost::throw_exception(UnsupportedOperationException());
}

QueryPtr Query::mergeBooleanQueries(Collection<BooleanQueryPtr> queries) {
    SetBooleanClause allClauses(SetBooleanClause::newInstance());
    for (Collection<BooleanQueryPtr>::iterator booleanQuery = queries.begin(); booleanQuery != queries.end(); ++booleanQuery) {
        for (Collection<BooleanClausePtr>::iterator clause = (*booleanQuery)->begin(); clause != (*booleanQuery)->end(); ++clause) {
            allClauses.add(*clause);
        }
    }

    bool coordDisabled = queries.empty() ? false : queries[0]->isCoordDisabled();
    BooleanQueryPtr result(newLucene<BooleanQuery>(coordDisabled));
    for (SetBooleanClause::iterator clause2 = allClauses.begin(); clause2 != allClauses.end(); ++clause2) {
        result->add(*clause2);
    }
    return result;
}

SimilarityPtr Query::getSimilarity(const SearcherPtr& searcher) {
    return searcher->getSimilarity();
}

LuceneObjectPtr Query::clone(const LuceneObjectPtr& other) {
    LuceneObjectPtr clone = LuceneObject::clone(other ? other : newLucene<Query>());
    QueryPtr cloneQuery(std::dynamic_pointer_cast<Query>(clone));
    cloneQuery->boost = boost;
    return cloneQuery;
}

int32_t Query::hashCode() {
    int32_t prime = 31;
    int32_t result = 1;
    result = prime * result + MiscUtils::doubleToIntBits(boost);
    return result;
}

bool Query::equals(const LuceneObjectPtr& other) {
    if (LuceneObject::equals(other)) {
        return true;
    }
    if (!other) {
        return false;
    }
    if (!MiscUtils::equalTypes(shared_from_this(), other)) {
        return false;
    }
    QueryPtr otherQuery(std::dynamic_pointer_cast<Query>(other));
    if (!otherQuery) {
        return false;
    }
    return (boost == otherQuery->boost);
}

String Query::boostString() {
    double boost = getBoost();
    if (boost == 1.0) {
        return L"";
    }
    StringStream boostString;
    boostString.precision(1);
    boostString.setf(std::ios::fixed);
    boostString << L"^" << boost;
    return boostString.str();
}

}

}
