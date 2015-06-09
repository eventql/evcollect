/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2009-2014 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
// or the GNU Lesser General Public License.
/////////////////////////////////////////////////////////////////////////////

#ifndef DOCCONSUMERPERTHREAD_H
#define DOCCONSUMERPERTHREAD_H

#include "fnord-fts/util/LuceneObject.h"

namespace fnord {
namespace fts {

class DocConsumerPerThread : public LuceneObject {
public:
    virtual ~DocConsumerPerThread();

    LUCENE_CLASS(DocConsumerPerThread);

public:
    /// Process the document. If there is something for this document to be done in docID order,
    /// you should encapsulate that as a DocWriter and return it.
    /// DocumentsWriter then calls finish() on this object when it's its turn.
    virtual DocWriterPtr processDocument() = 0;

    virtual void abort() = 0;
};

}

}
#endif
