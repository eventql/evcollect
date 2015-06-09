/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2009-2014 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
// or the GNU Lesser General Public License.
/////////////////////////////////////////////////////////////////////////////

#include "fnord-fts/fts.h"
#include "fnord-fts/search/payloads/PayloadSpanUtil.h"
#include "fnord-fts/search/BooleanQuery.h"
#include "fnord-fts/search/BooleanClause.h"
#include "fnord-fts/search/PhraseQuery.h"
#include "fnord-fts/search/spans/SpanTermQuery.h"
#include "fnord-fts/search/spans/SpanNearQuery.h"
#include "fnord-fts/search/spans/SpanOrQuery.h"
#include "fnord-fts/search/TermQuery.h"
#include "fnord-fts/search/FilteredQuery.h"
#include "fnord-fts/search/DisjunctionMaxQuery.h"
#include "fnord-fts/search/MultiPhraseQuery.h"
#include "fnord-fts/index/Term.h"
#include "fnord-fts/search/spans/Spans.h"
#include "fnord-fts/util/MiscUtils.h"

namespace fnord {
namespace fts {

PayloadSpanUtil::PayloadSpanUtil(const IndexReaderPtr& reader) {
    this->reader = reader;
}

PayloadSpanUtil::~PayloadSpanUtil() {
}

Collection<ByteArray> PayloadSpanUtil::getPayloadsForQuery(const QueryPtr& query) {
    Collection<ByteArray> payloads(Collection<ByteArray>::newInstance());
    queryToSpanQuery(query, payloads);
    return payloads;
}

void PayloadSpanUtil::queryToSpanQuery(const QueryPtr& query, Collection<ByteArray> payloads) {
    if (MiscUtils::typeOf<BooleanQuery>(query)) {
        BooleanQueryPtr booleanQuery(std::dynamic_pointer_cast<BooleanQuery>(query));
        Collection<BooleanClausePtr> queryClauses(booleanQuery->getClauses());
        for (Collection<BooleanClausePtr>::iterator clause = queryClauses.begin(); clause != queryClauses.end(); ++clause) {
            if (!(*clause)->isProhibited()) {
                queryToSpanQuery((*clause)->getQuery(), payloads);
            }
        }
    } else if (MiscUtils::typeOf<PhraseQuery>(query)) {
        PhraseQueryPtr phraseQuery(std::dynamic_pointer_cast<PhraseQuery>(query));
        Collection<TermPtr> phraseQueryTerms(phraseQuery->getTerms());
        Collection<SpanQueryPtr> clauses(Collection<SpanQueryPtr>::newInstance(phraseQueryTerms.size()));
        for (int32_t i = 0; i < phraseQueryTerms.size(); ++i) {
            clauses[i] = newLucene<SpanTermQuery>(phraseQueryTerms[i]);
        }

        int32_t slop = phraseQuery->getSlop();
        bool inorder = false;

        if (slop == 0) {
            inorder = true;
        }

        SpanNearQueryPtr sp(newLucene<SpanNearQuery>(clauses, slop, inorder));
        sp->setBoost(query->getBoost());
        getPayloads(payloads, sp);
    } else if (MiscUtils::typeOf<TermQuery>(query)) {
        TermQueryPtr termQuery(std::dynamic_pointer_cast<TermQuery>(query));
        SpanTermQueryPtr stq(newLucene<SpanTermQuery>(termQuery->getTerm()));
        stq->setBoost(query->getBoost());
        getPayloads(payloads, stq);
    } else if (MiscUtils::typeOf<SpanQuery>(query)) {
        SpanQueryPtr spanQuery(std::dynamic_pointer_cast<SpanQuery>(query));
        getPayloads(payloads, spanQuery);
    } else if (MiscUtils::typeOf<FilteredQuery>(query)) {
        FilteredQueryPtr filteredQuery(std::dynamic_pointer_cast<FilteredQuery>(query));
        queryToSpanQuery(filteredQuery->getQuery(), payloads);
    } else if (MiscUtils::typeOf<DisjunctionMaxQuery>(query)) {
        DisjunctionMaxQueryPtr maxQuery(std::dynamic_pointer_cast<DisjunctionMaxQuery>(query));
        for (Collection<QueryPtr>::iterator disjunct = maxQuery->begin(); disjunct != maxQuery->end(); ++disjunct) {
            queryToSpanQuery(*disjunct, payloads);
        }
    } else if (MiscUtils::typeOf<MultiPhraseQuery>(query)) {
        MultiPhraseQueryPtr multiphraseQuery(std::dynamic_pointer_cast<MultiPhraseQuery>(query));
        Collection< Collection<TermPtr> > termArrays(multiphraseQuery->getTermArrays());
        Collection<int32_t> positions(multiphraseQuery->getPositions());
        if (!positions.empty()) {
            int32_t maxPosition = positions[positions.size() - 1];
            for (int32_t i = 0; i < positions.size() - 1; ++i) {
                if (positions[i] > maxPosition) {
                    maxPosition = positions[i];
                }
            }

            Collection< Collection<QueryPtr> > disjunctLists(Collection< Collection<QueryPtr> >::newInstance(maxPosition + 1));
            int32_t distinctPositions = 0;

            for (int32_t i = 0; i < termArrays.size(); ++i) {
                Collection<TermPtr> termArray(termArrays[i]);
                Collection<QueryPtr> disjuncts(disjunctLists[positions[i]]);
                if (!disjuncts) {
                    disjuncts = Collection<QueryPtr>::newInstance();
                    disjunctLists[positions[i]] = disjuncts;
                    ++distinctPositions;
                }
                for (Collection<TermPtr>::iterator term = termArray.begin(); term != termArray.end(); ++term) {
                    disjuncts.add(newLucene<SpanTermQuery>(*term));
                }
            }

            int32_t positionGaps = 0;
            int32_t position = 0;
            Collection<SpanQueryPtr> clauses(Collection<SpanQueryPtr>::newInstance(distinctPositions));
            for (int32_t i = 0; i < disjunctLists.size(); ++i) {
                Collection<QueryPtr> disjuncts(disjunctLists[i]);
                if (disjuncts) {
                    Collection<SpanQueryPtr> spanDisjuncts(Collection<SpanQueryPtr>::newInstance(disjuncts.size()));
                    for (int32_t j = 0; j < disjuncts.size(); ++j) {
                        spanDisjuncts[j] = std::dynamic_pointer_cast<SpanQuery>(disjuncts[j]);
                    }
                    clauses[position++] = newLucene<SpanOrQuery>(spanDisjuncts);
                } else {
                    ++positionGaps;
                }
            }

            int32_t slop = multiphraseQuery->getSlop();
            bool inorder = (slop == 0);

            SpanNearQueryPtr sp(newLucene<SpanNearQuery>(clauses, slop + positionGaps, inorder));
            sp->setBoost(query->getBoost());
            getPayloads(payloads, sp);
        }
    }
}

void PayloadSpanUtil::getPayloads(Collection<ByteArray> payloads, const SpanQueryPtr& query) {
    SpansPtr spans(query->getSpans(reader));

    while (spans->next()) {
        if (spans->isPayloadAvailable()) {
            Collection<ByteArray> payload(spans->getPayload());
            for (Collection<ByteArray>::iterator bytes = payload.begin(); bytes != payload.end(); ++bytes) {
                payloads.add(*bytes);
            }
        }
    }
}

}

}
