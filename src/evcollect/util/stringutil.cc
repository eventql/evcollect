/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include <string>
#include <evcollect/util/stringutil.h>

void StringUtil::toStringVImpl(std::vector<std::string>* target) {}

template <>
std::string StringUtil::toString(std::string value) {
  return value;
}

template <>
std::string StringUtil::toString(const char* value) {
  return value;
}

template <>
std::string StringUtil::toString(char* value) {
  return value;
}

template <>
std::string StringUtil::toString(int value) {
  return std::to_string(value);
}

template <>
std::string StringUtil::toString(unsigned value) {
  return std::to_string(value);
}

template <>
std::string StringUtil::toString(unsigned short value) {
  return std::to_string(value);
}


template <>
std::string StringUtil::toString(long value) {
  return std::to_string(value);
}

template <>
std::string StringUtil::toString(unsigned long value) {
  return std::to_string(value);
}

template <>
std::string StringUtil::toString(long long value) {
  return std::to_string(value);
}

template <>
std::string StringUtil::toString(unsigned long long value) {
  return std::to_string(value);
}

template <>
std::string StringUtil::toString(unsigned char value) {
  return std::to_string(value);
}

template <>
std::string StringUtil::toString(void* value) {
  return "<ptr>";
}

template <>
std::string StringUtil::toString(const void* value) {
  return "<ptr>";
}

template <>
std::string StringUtil::toString(double value) {
  char buf[128]; // FIXPAUL
  *buf = 0;

  auto len = snprintf(buf, sizeof(buf), "%f", value);
  if (len < 0) {
    return std::string();
  }

  while (len > 2 && buf[len - 1] == '0' && buf[len - 2] != '.') {
    buf[len--] = 0;
  }

  return std::string(buf, len);
}

template <>
std::string StringUtil::toString(bool value) {
  return value ? "true" : "false";
}

void StringUtil::stripTrailingSlashes(std::string* str) {
  while (str->back() == '/') {
    str->pop_back();
  }
}

void StringUtil::replaceAll(
    std::string* str,
    const std::string& pattern,
    const std::string& replacement) {
  if (str->size() == 0) {
    return;
  }

  auto cur = 0;
  while((cur = str->find(pattern, cur)) != std::string::npos) {
    str->replace(cur, pattern.size(), replacement);
    cur += replacement.size();
  }
}

std::vector<std::string> StringUtil::split(
      const std::string& str,
      const std::string& pattern) {
  std::vector<std::string> parts;

  size_t begin = 0;
  for (;;) {
    auto end = str.find(pattern, begin);

    if (end == std::string::npos) {
      parts.emplace_back(str.substr(begin, end));
      break;
    } else {
      parts.emplace_back(str.substr(begin, end - begin));
      begin = end + pattern.length();
    }
  }

  return parts;
}

std::string StringUtil::join(
    const std::vector<std::string>& list,
    const std::string& join) {
  std::string out;

  for (int i = 0; i < list.size(); ++i) {
    if (i > 0) {
      out += join;
    }

    out += list[i];
  }

  return out;
}

std::string StringUtil::join(
    const std::set<std::string>& list,
    const std::string& join) {
  std::string out;

  size_t i = 0;
  for (const auto& item : list) {
    if (++i > 1) {
      out += join;
    }

    out += item;
  }

  return out;
}

bool StringUtil::beginsWith(const std::string& str, const std::string& prefix) {
  if (str.length() < prefix.length()) {
    return false;
  }

  return str.compare(
      0,
      prefix.length(),
      prefix) == 0;
}


bool StringUtil::endsWith(const std::string& str, const std::string& suffix) {
  if (str.length() < suffix.length()) {
    return false;
  }

  return str.compare(
      str.length() - suffix.length(),
      suffix.length(),
      suffix) == 0;
}

int StringUtil::compare(
    const char* s1,
    size_t s1_len,
    const char* s2,
    size_t s2_len) {
  for (; s1_len > 0 && s2_len > 0; s1++, s2++, --s1_len, --s2_len) {
    if (*s1 != *s2) {
      return (*(uint8_t *) s1 < *(uint8_t *) s2) ? -1 : 1;
    }
  }

  if (s1_len > 0) {
    return 1;
  }

  if (s2_len > 0) {
    return -1;
  }

  return 0;
}


bool StringUtil::isHexString(const std::string& str) {
  for (const auto& c : str) {
    if ((c >= '0' && c <= '9') ||
        (c >= 'a' && c <= 'f') ||
        (c >= 'A' && c <= 'F')) {
      continue;
    }

    return false;
  }

  return true;
}

bool StringUtil::isAlphanumeric(const std::string& str) {
  for (const auto& c : str) {
    if (!isAlphanumeric(c)) {
      return false;
    }
  }

  return true;
}

bool StringUtil::isAlphanumeric(char chr) {
  bool is_alphanum =
      (chr >= '0' && chr <= '9') ||
      (chr >= 'a' && chr <= 'z') ||
      (chr >= 'A' && chr <= 'Z');

  return is_alphanum;
}

bool StringUtil::isShellSafe(const std::string& str) {
  for (const auto& c : str) {
    if (!isShellSafe(c)) {
      return false;
    }
  }

  return true;
}

bool StringUtil::isShellSafe(char chr) {
  bool is_safe =
      (chr >= '0' && chr <= '9') ||
      (chr >= 'a' && chr <= 'z') ||
      (chr >= 'A' && chr <= 'Z') ||
      (chr == '_') ||
      (chr == '-') ||
      (chr == '.');

  return is_safe;
}

bool StringUtil::isDigitString(const std::string& str) {
  return isDigitString(str.data(), str.data() + str.size());
}

bool StringUtil::isDigitString(const char* begin, const char* end) {
  for (auto cur = begin; cur < end; ++cur) {
    if (!isdigit(*cur)) {
      return false;
    }
  }

  return true;
}

bool StringUtil::isNumber(const std::string& str) {
  return isNumber(str.data(), str.data() + str.size());
}

bool StringUtil::isNumber(const char* begin, const char* end) {
  auto cur = begin;

  if (cur < end && *cur == '-') {
    ++cur;
  }

  for (; cur < end; ++cur) {
    if (!isdigit(*cur)) {
      return false;
    }
  }

  if (cur < end && (*cur == '.' || *cur == ',')) {
    ++cur;
  }

  for (; cur < end; ++cur) {
    if (!isdigit(*cur)) {
      return false;
    }
  }

  return true;
}

void StringUtil::toLower(std::string* str) {
  auto& str_ref = *str;

  for (int i = 0; i < str_ref.length(); ++i) {
    str_ref[i] = std::tolower(str_ref[i]);
  }
}

void StringUtil::toUpper(std::string* str) {
  auto& str_ref = *str;

  for (int i = 0; i < str_ref.length(); ++i) {
    str_ref[i] = std::toupper(str_ref[i]);
  }
}

size_t StringUtil::find(const std::string& str, char chr) {
  for (int i = 0; i < str.length(); ++i) {
    if (str[i] == chr) {
      return i;
    }
  }

  return -1;
}

size_t StringUtil::findLast(const std::string& str, char chr) {
  for (int i = str.length() - 1; i >= 0; --i) {
    if (str[i] == chr) {
      return i;
    }
  }

  return -1;
}

bool StringUtil::includes(const std::string& str, const std::string& subject) {
  return str.find(subject) != std::string::npos;
}

bool StringUtil::includesi(const std::string& str, const std::string& subject) {
  auto str_i = str;
  StringUtil::toLower(&str_i);
  auto subject_i = subject;
  StringUtil::toLower(&subject_i);
  return includes(str_i, subject_i);
}

std::string StringUtil::formatv(
    const char* fmt,
    std::vector<std::string> values) {
  std::string str = fmt;

  for (int i = 0; i < values.size(); ++i) {
    StringUtil::replaceAll(
        &str,
        "$" + std::to_string(i),
        StringUtil::toString(values[i]));
  }

  return str;
}

std::string StringUtil::stripShell(const std::string& str) {
  std::string out;

  for (const auto& c : str) {
    if (isAlphanumeric(c) || c == '_' || c == '-' || c == '.') {
      out += c;
    }
  }

  return out;
}

