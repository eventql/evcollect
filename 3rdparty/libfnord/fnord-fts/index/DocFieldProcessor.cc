/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2009-2014 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
// or the GNU Lesser General Public License.
/////////////////////////////////////////////////////////////////////////////

#include "fnord-fts/fts.h"
#include "fnord-fts/index/DocFieldProcessor.h"
#include "fnord-fts/index/DocFieldProcessorPerThread.h"
#include "fnord-fts/index/DocFieldConsumerPerThread.h"
#include "fnord-fts/index/DocFieldConsumer.h"
#include "fnord-fts/index/StoredFieldsWriter.h"
#include "fnord-fts/index/SegmentWriteState.h"
#include "fnord-fts/index/IndexFileNames.h"
#include "fnord-fts/index/FieldInfos.h"
#include "fnord-fts/util/TestPoint.h"

namespace fnord {
namespace fts {

DocFieldProcessor::DocFieldProcessor(const DocumentsWriterPtr& docWriter, const DocFieldConsumerPtr& consumer) {
    this->fieldInfos = newLucene<FieldInfos>();
    this->_docWriter = docWriter;
    this->consumer = consumer;
    consumer->setFieldInfos(fieldInfos);
    fieldsWriter = newLucene<StoredFieldsWriter>(docWriter, fieldInfos);
}

DocFieldProcessor::~DocFieldProcessor() {
}

void DocFieldProcessor::closeDocStore(const SegmentWriteStatePtr& state) {
    consumer->closeDocStore(state);
    fieldsWriter->closeDocStore(state);
}

void DocFieldProcessor::flush(Collection<DocConsumerPerThreadPtr> threads, const SegmentWriteStatePtr& state) {
    TestScope testScope(L"DocFieldProcessor", L"flush");
    MapDocFieldConsumerPerThreadCollectionDocFieldConsumerPerField childThreadsAndFields(MapDocFieldConsumerPerThreadCollectionDocFieldConsumerPerField::newInstance());

    for (Collection<DocConsumerPerThreadPtr>::iterator thread = threads.begin(); thread != threads.end(); ++thread) {
        DocFieldProcessorPerThreadPtr perThread(std::static_pointer_cast<DocFieldProcessorPerThread>(*thread));
        childThreadsAndFields.put(perThread->consumer, perThread->fields());
        perThread->trimFields(state);
    }
    fieldsWriter->flush(state);
    consumer->flush(childThreadsAndFields, state);

    // Important to save after asking consumer to flush so consumer can alter the FieldInfo* if necessary.
    // eg FreqProxTermsWriter does this with FieldInfo.storePayload.
    String fileName(state->segmentFileName(IndexFileNames::FIELD_INFOS_EXTENSION()));
    fieldInfos->write(state->directory, fileName);
    state->flushedFiles.add(fileName);
}

void DocFieldProcessor::abort() {
    fieldsWriter->abort();
    consumer->abort();
}

bool DocFieldProcessor::freeRAM() {
    return consumer->freeRAM();
}

DocConsumerPerThreadPtr DocFieldProcessor::addThread(const DocumentsWriterThreadStatePtr& perThread) {
    return newLucene<DocFieldProcessorPerThread>(perThread, shared_from_this());
}

}

}
