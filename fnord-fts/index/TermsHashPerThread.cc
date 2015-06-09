/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2009-2014 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
// or the GNU Lesser General Public License.
/////////////////////////////////////////////////////////////////////////////

#include "fnord-fts/fts.h"
#include "fnord-fts/index/TermsHashPerThread.h"
#include "fnord-fts/index/TermsHashPerField.h"
#include "fnord-fts/index/DocInverterPerThread.h"
#include "fnord-fts/index/TermsHash.h"
#include "fnord-fts/index/TermsHashConsumer.h"
#include "fnord-fts/index/TermsHashConsumerPerThread.h"
#include "fnord-fts/index/CharBlockPool.h"
#include "fnord-fts/index/IntBlockPool.h"
#include "fnord-fts/index/DocumentsWriter.h"

namespace fnord {
namespace fts {

TermsHashPerThread::TermsHashPerThread(const DocInverterPerThreadPtr& docInverterPerThread, const TermsHashPtr& termsHash, const TermsHashPtr& nextTermsHash, const TermsHashPerThreadPtr& primaryPerThread) {
    this->freePostings = Collection<RawPostingListPtr>::newInstance(256);
    this->freePostingsCount = 0;
    this->primary = false;
    this->_docInverterPerThread = docInverterPerThread;
    this->_termsHash = termsHash;
    this->nextTermsHash = nextTermsHash;
    this->_primaryPerThread = primaryPerThread;
}

TermsHashPerThread::~TermsHashPerThread() {
}

void TermsHashPerThread::initialize() {
    DocInverterPerThreadPtr docInverterPerThread(_docInverterPerThread);
    TermsHashPtr termsHash(_termsHash);
    docState = docInverterPerThread->docState;
    consumer = termsHash->consumer->addThread(shared_from_this());

    if (nextTermsHash) {
        // We are primary
        charPool = newLucene<CharBlockPool>(DocumentsWriterPtr(termsHash->_docWriter));
        primary = true;
    } else {
        charPool = TermsHashPerThreadPtr(_primaryPerThread)->charPool;
        primary = false;
    }

    intPool = newLucene<IntBlockPool>(DocumentsWriterPtr(termsHash->_docWriter), termsHash->trackAllocations);
    bytePool = newLucene<ByteBlockPool>(DocumentsWriterPtr(termsHash->_docWriter)->byteBlockAllocator, termsHash->trackAllocations);

    if (nextTermsHash) {
        nextPerThread = nextTermsHash->addThread(docInverterPerThread, shared_from_this());
    }
}

InvertedDocConsumerPerFieldPtr TermsHashPerThread::addField(const DocInverterPerFieldPtr& docInverterPerField, const FieldInfoPtr& fieldInfo) {
    return newLucene<TermsHashPerField>(docInverterPerField, shared_from_this(), nextPerThread, fieldInfo);
}

void TermsHashPerThread::abort() {
    SyncLock syncLock(this);
    reset(true);
    consumer->abort();
    if (nextPerThread) {
        nextPerThread->abort();
    }
}

void TermsHashPerThread::morePostings() {
    BOOST_ASSERT(freePostingsCount == 0);
    TermsHashPtr(_termsHash)->getPostings(freePostings);
    freePostingsCount = freePostings.size();
    BOOST_ASSERT(noNullPostings(freePostings, freePostingsCount, L"consumer=" + consumer->toString()));
}

bool TermsHashPerThread::noNullPostings(Collection<RawPostingListPtr> postings, int32_t count, const String& details) {
    for (int32_t i = 0; i < count; ++i) {
        BOOST_ASSERT(postings[i]);
    }
    return true;
}

void TermsHashPerThread::startDocument() {
    consumer->startDocument();
    if (nextPerThread) {
        nextPerThread->consumer->startDocument();
    }
}

DocWriterPtr TermsHashPerThread::finishDocument() {
    DocWriterPtr doc(consumer->finishDocument());
    DocWriterPtr doc2(nextPerThread ? nextPerThread->consumer->finishDocument() : DocWriterPtr());
    if (!doc) {
        return doc2;
    } else {
        doc->setNext(doc2);
        return doc;
    }
}

void TermsHashPerThread::reset(bool recyclePostings) {
    intPool->reset();
    bytePool->reset();

    if (primary) {
        charPool->reset();
    }

    if (recyclePostings) {
        TermsHashPtr(_termsHash)->recyclePostings(freePostings, freePostingsCount);
        freePostingsCount = 0;
    }
}

}

}
