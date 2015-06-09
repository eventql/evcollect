/*
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
* FnordMetric is free software: you can redistribute it and/or modify it under
* the terms of the GNU General Public License v3.0. You should have received a
* copy of the GNU General Public License along with this program. If not, see
* <http://www.gnu.org/licenses/>.
*/
#include "fnord-base/inspect.h"
#include "fnord-base/UTF8.h"
#include "fnord-fts/fts.h"
#include "fnord-fts/fts_common.h"
#include "fnord-fts/document/Fieldable.h"
#include "fnord-fts/AnalyzerAdapter.h"

namespace fnord {
namespace fts {

AnalyzerAdapter::AnalyzerAdapter(
    RefPtr<Analyzer> analyzer) :
    analyzer_(analyzer) {}

AnalyzerAdapter::~AnalyzerAdapter() {}

TokenStreamPtr AnalyzerAdapter::reusableTokenStream(
    const String& fieldName,
    const ReaderPtr& reader) {
  return tokenStream(fieldName, reader);
}

LuceneObjectPtr AnalyzerAdapter::getPreviousTokenStream() {
  return tokenStreams.get();
}

void AnalyzerAdapter::setPreviousTokenStream(const LuceneObjectPtr& stream) {
  tokenStreams.set(stream);
}

int32_t AnalyzerAdapter::getPositionIncrementGap(const String& fieldName) {
  return 0;
}

int32_t AnalyzerAdapter::getOffsetGap(const FieldablePtr& field) {
  return field->isTokenized() ? 1 : 0;
}

void AnalyzerAdapter::close() {
  tokenStreams.close();
}

TokenStreamPtr AnalyzerAdapter::tokenStream(
    const String& field_name16,
    const ReaderPtr& reader) {
  std::string field_name = StringUtil::convertUTF16To8(field_name16);
  std::string field_str;
  wchar_t chr;
  while ((chr = reader->read()) > 0) {
    UTF8::encodeCodepoint(chr, &field_str);
  }

  auto ts = std::make_shared<TokenStream>();
  auto lang = Language::DE;
  analyzer_->extractTerms(lang, field_str, [&ts] (const fnord::String& t) {
    ts->addToken(t);
  });

  return ts;
}

}
}
