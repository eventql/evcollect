/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2009-2014 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
// or the GNU Lesser General Public License.
/////////////////////////////////////////////////////////////////////////////

#include "fnord-fts/fts.h"
#include "fnord-fts/index/StoredFieldsWriterPerThread.h"
#include "fnord-fts/index/StoredFieldsWriter.h"
#include "fnord-fts/index/FieldsWriter.h"
#include "fnord-fts/store/RAMOutputStream.h"

namespace fnord {
namespace fts {

StoredFieldsWriterPerThread::StoredFieldsWriterPerThread(const DocStatePtr& docState, const StoredFieldsWriterPtr& storedFieldsWriter) {
    this->_storedFieldsWriter = storedFieldsWriter;
    this->docState = docState;
    localFieldsWriter = newLucene<FieldsWriter>(IndexOutputPtr(), IndexOutputPtr(), storedFieldsWriter->fieldInfos);
}

StoredFieldsWriterPerThread::~StoredFieldsWriterPerThread() {
}

void StoredFieldsWriterPerThread::startDocument() {
    if (doc) {
        // Only happens if previous document hit non-aborting exception while writing stored fields
        // into localFieldsWriter
        doc->reset();
        doc->docID = docState->docID;
    }
}

void StoredFieldsWriterPerThread::addField(const FieldablePtr& field, const FieldInfoPtr& fieldInfo) {
    if (!doc) {
        doc = StoredFieldsWriterPtr(_storedFieldsWriter)->getPerDoc();
        doc->docID = docState->docID;
        localFieldsWriter->setFieldsStream(doc->fdt);
        BOOST_ASSERT(doc->numStoredFields == 0);
        BOOST_ASSERT(doc->fdt->length() == 0);
        BOOST_ASSERT(doc->fdt->getFilePointer() == 0);
    }

    localFieldsWriter->writeField(fieldInfo, field);
    BOOST_ASSERT(docState->testPoint(L"StoredFieldsWriterPerThread.processFields.writeField"));
    ++doc->numStoredFields;
}

DocWriterPtr StoredFieldsWriterPerThread::finishDocument() {
    // If there were any stored fields in this doc, doc will be non-null; else it's null.
    DocWriterPtr finishDoc(doc);
    doc.reset();
    return finishDoc;
}

void StoredFieldsWriterPerThread::abort() {
    if (doc) {
        doc->abort();
        doc.reset();
    }
}

}

}
