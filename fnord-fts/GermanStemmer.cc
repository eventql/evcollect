/*
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "fnord-base/UTF8.h"
#include "fnord-fts/GermanStemmer.h"

namespace fnord {
namespace fts {

GermanStemmer::GermanStemmer(
      const fnord::String& hunspell_aff_file,
      const fnord::String& hunspell_dict_file,
      const fnord::String& hunspell_hyphen_file,
      SynonymDictionary* synonyms) :
      hunspell_(hunspell_aff_file, hunspell_dict_file, hunspell_hyphen_file),
      synonyms_(synonyms) {}

void GermanStemmer::stem(Language lang, fnord::String* term) {
  stemWithUmlauts(lang, term);
  removeUmlauts(term);
}

void GermanStemmer::stemWithUmlauts(Language lang, fnord::String* term) {
  auto stem = findStemFor(lang, *term);
  if (!stem.isEmpty()) {
    term->assign(stem.get());
    return;
  }

  for (auto p : hunspell_.hyphenate(*term)) {
    auto primary_compound = term->substr(p + 1);

    auto stem = findStemFor(lang, primary_compound);
    if (!stem.isEmpty()) {
      term->assign(term->substr(0, p + 1) + stem.get());
      return;
    }
  }
}

Option<fnord::String> GermanStemmer::findStemFor(Language lang, const fnord::String& term) {
  auto synonym =  synonyms_->lookup(lang, term);
  if (!synonym.isEmpty()) {
    return synonym;
  }

  auto stem = hunspell_.stem(term);
  if (!stem.isEmpty()) {
    return stem;
  }

  return None<fnord::String>();
}

void GermanStemmer::removeUmlauts(fnord::String* term) {
  fnord::String stripped;
  stripped.reserve(term->size() + 2);

  auto cur = term->data();
  auto end = cur + term->size();
  while (cur < end) {
    auto chr = UTF8::nextCodepoint(&cur, end);
    switch (chr) {

      case L'ä':
        stripped += "ae";
        break;

      case L'ö':
        stripped += "oe";
        break;

      case L'ü':
        stripped += "ue";
        break;

      case L'Ä':
        stripped += "ae";
        break;

      case L'Ö':
        stripped += "oe";
        break;

      case L'Ü':
        stripped += "ue";
        break;

      case L'ß':
        stripped += "ss";
        break;

      default:
        UTF8::encodeCodepoint(chr, &stripped);
        break;

    }
  }

  term->assign(stripped);
}

} // namespace fts
} // namespace fnord
