/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2009-2014 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
// or the GNU Lesser General Public License.
/////////////////////////////////////////////////////////////////////////////

#include "fnord-fts/fts.h"
#include "fnord-fts/index/DocInverter.h"
#include "fnord-fts/index/InvertedDocConsumer.h"
#include "fnord-fts/index/InvertedDocEndConsumer.h"
#include "fnord-fts/index/InvertedDocConsumerPerThread.h"
#include "fnord-fts/index/InvertedDocEndConsumerPerThread.h"
#include "fnord-fts/index/DocFieldConsumerPerThread.h"
#include "fnord-fts/index/DocFieldConsumerPerField.h"
#include "fnord-fts/index/DocInverterPerField.h"
#include "fnord-fts/index/DocInverterPerThread.h"

namespace fnord {
namespace fts {

DocInverter::DocInverter(const InvertedDocConsumerPtr& consumer, const InvertedDocEndConsumerPtr& endConsumer) {
    this->consumer = consumer;
    this->endConsumer = endConsumer;
}

DocInverter::~DocInverter() {
}

void DocInverter::setFieldInfos(const FieldInfosPtr& fieldInfos) {
    DocFieldConsumer::setFieldInfos(fieldInfos);
    consumer->setFieldInfos(fieldInfos);
    endConsumer->setFieldInfos(fieldInfos);
}

void DocInverter::flush(MapDocFieldConsumerPerThreadCollectionDocFieldConsumerPerField threadsAndFields, const SegmentWriteStatePtr& state) {
    MapInvertedDocConsumerPerThreadCollectionInvertedDocConsumerPerField childThreadsAndFields(MapInvertedDocConsumerPerThreadCollectionInvertedDocConsumerPerField::newInstance());
    MapInvertedDocEndConsumerPerThreadCollectionInvertedDocEndConsumerPerField endChildThreadsAndFields(MapInvertedDocEndConsumerPerThreadCollectionInvertedDocEndConsumerPerField::newInstance());

    for (MapDocFieldConsumerPerThreadCollectionDocFieldConsumerPerField::iterator entry = threadsAndFields.begin(); entry != threadsAndFields.end(); ++entry) {
        Collection<InvertedDocConsumerPerFieldPtr> childFields(Collection<InvertedDocConsumerPerFieldPtr>::newInstance());
        Collection<InvertedDocEndConsumerPerFieldPtr> endChildFields(Collection<InvertedDocEndConsumerPerFieldPtr>::newInstance());

        for (Collection<DocFieldConsumerPerFieldPtr>::iterator perField = entry->second.begin(); perField != entry->second.end(); ++perField) {
            childFields.add(std::static_pointer_cast<DocInverterPerField>(*perField)->consumer);
            endChildFields.add(std::static_pointer_cast<DocInverterPerField>(*perField)->endConsumer);
        }

        childThreadsAndFields.put(std::static_pointer_cast<DocInverterPerThread>(entry->first)->consumer, childFields);
        endChildThreadsAndFields.put(std::static_pointer_cast<DocInverterPerThread>(entry->first)->endConsumer, endChildFields);
    }

    consumer->flush(childThreadsAndFields, state);
    endConsumer->flush(endChildThreadsAndFields, state);
}

void DocInverter::closeDocStore(const SegmentWriteStatePtr& state) {
    consumer->closeDocStore(state);
    endConsumer->closeDocStore(state);
}

void DocInverter::abort() {
    consumer->abort();
    endConsumer->abort();
}

bool DocInverter::freeRAM() {
    return consumer->freeRAM();
}

DocFieldConsumerPerThreadPtr DocInverter::addThread(const DocFieldProcessorPerThreadPtr& docFieldProcessorPerThread) {
    return newLucene<DocInverterPerThread>(docFieldProcessorPerThread, shared_from_this());
}

}

}
