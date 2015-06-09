/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2009-2014 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
// or the GNU Lesser General Public License.
/////////////////////////////////////////////////////////////////////////////

#ifndef DISJUNCTIONMAXSCORER_H
#define DISJUNCTIONMAXSCORER_H

#include "fnord-fts/search/Scorer.h"

namespace fnord {
namespace fts {

/// The Scorer for DisjunctionMaxQuery.  The union of all documents generated by the the subquery scorers
/// is generated in document number order.  The score for each document is the maximum of the scores computed
/// by the subquery scorers that generate that document, plus tieBreakerMultiplier times the sum of the scores
/// for the other subqueries that generate the document.
class DisjunctionMaxScorer : public Scorer {
public:
    DisjunctionMaxScorer(double tieBreakerMultiplier, const SimilarityPtr& similarity, Collection<ScorerPtr> subScorers, int32_t numScorers);
    virtual ~DisjunctionMaxScorer();

    LUCENE_CLASS(DisjunctionMaxScorer);

protected:
    /// The scorers for subqueries that have remaining docs, kept as a min heap by number of next doc.
    Collection<ScorerPtr> subScorers;
    int32_t numScorers;

    /// Multiplier applied to non-maximum-scoring subqueries for a document as they are summed into the result.
    double tieBreakerMultiplier;

    int32_t doc;

public:
    virtual int32_t nextDoc();
    virtual int32_t docID();

    /// Determine the current document score.  Initially invalid, until {@link #next()} is called the first time.
    /// @return the score of the current generated document
    virtual double score();

    virtual int32_t advance(int32_t target);

protected:
    /// Recursively iterate all subScorers that generated last doc computing sum and max
    void scoreAll(int32_t root, int32_t size, int32_t doc, Collection<double> sum, Collection<double> max);

    /// Organize subScorers into a min heap with scorers generating the earliest document on top.
    void heapify();

    /// The subtree of subScorers at root is a min heap except possibly for its root element.  Bubble the root
    /// down as required to make the subtree a heap.
    void heapAdjust(int32_t root);

    /// Remove the root Scorer from subScorers and re-establish it as a heap
    void heapRemoveRoot();
};

}

}
#endif
