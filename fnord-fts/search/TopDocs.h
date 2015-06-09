/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2009-2014 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
// or the GNU Lesser General Public License.
/////////////////////////////////////////////////////////////////////////////

#ifndef TOPDOCS_H
#define TOPDOCS_H

#include "fnord-base/stdtypes.h"
#include "fnord-fts/util/LuceneObject.h"

namespace fnord {
namespace fts {

/// Represents hits returned by {@link Searcher#search(QueryPtr, FilterPtr, int32_t)} and {@link
/// Searcher#search(QueryPtr, int32_t)}.
class TopDocs : public LuceneObject {
public:
    /// Constructs a TopDocs with a default maxScore = double.NaN.
    TopDocs(int32_t totalHits, Collection<ScoreDocPtr> scoreDocs);

    /// Constructs a TopDocs.
    TopDocs(int32_t totalHits, Collection<ScoreDocPtr> scoreDocs, double maxScore);

    virtual ~TopDocs();

    LUCENE_CLASS(TopDocs);

    /// The total number of hits for the query.
    int32_t totalHits;

    /// The top hits for the query.
    Collection<ScoreDocPtr> scoreDocs;

    /// Stores the maximum score value encountered, needed for normalizing.
    double maxScore;

    /// Returns the maximum score value encountered. Note that in case scores are not tracked,
    /// this returns NaN.
    double getMaxScore();

    /// Sets the maximum score value encountered.
    void setMaxScore(double maxScore);

    void forEach(Function<bool (ScoreDoc* sdoc)> fn) const;

};

}

}
#endif
