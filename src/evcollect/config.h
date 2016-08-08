/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Christian Parpart <christianparpart@googlemail.io>
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
#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <evcollect/evcollect.h>
#include <evcollect/util/return_code.h>

namespace evcollect {

struct PropertyList {
  std::vector<std::pair<std::string, std::vector<std::string>>> properties;
  bool get(const std::string& key, std::string* out) const;
  bool get(const std::string& key, const char** out) const;
  bool getv(const std::string& key, size_t i, size_t j, const char** out) const;
  size_t get(
      const std::string& key,
      std::vector<std::vector<std::string>>* out) const;
};

struct EventSourceBindingConfig {
  std::string plugin_name;
  std::string plugin_value;
  PropertyList properties;
};

struct EventBindingConfig {
  std::string event_name;
  uint64_t interval_micros;
  std::vector<EventSourceBindingConfig> sources;
};

struct TargetBindingConfig {
  std::string plugin_name;
  std::string plugin_value;
  PropertyList properties;
};

struct ProcessConfig {
  std::vector<EventBindingConfig> event_bindings;
  std::vector<TargetBindingConfig> target_bindings;
  std::string spool_dir;
  std::string plugin_dir;
  std::vector<std::string> load_plugins;
};

ReturnCode loadConfig(
    const std::string& config_file_path,
    ProcessConfig* config);

enum class ConfigToken {
  Unknown,
  Eof,
  End,
  Name,
  Value,
  Event,
  Source,
  Output,
  Plugin,
};

class ConfigLexer {
 public:
  ConfigLexer(const std::string& label, std::unique_ptr<std::istream>&& input);

  static std::unique_ptr<ConfigLexer> fromString(const std::string& config);
  static std::unique_ptr<ConfigLexer> fromLocalFile(const std::string& path);

  bool eof() const;
  ConfigToken nextToken();

  ConfigToken currentToken() const noexcept;
  int currentLineNr() const noexcept;
  int currentColumn() const noexcept;
  std::string currentContext() const;

  std::string stringValue() const;

 private:
  int currentChar() const noexcept;
  int peekChar();
  int nextChar();
  bool consumeSpace();
  bool consumeEndOfLine();

  ConfigToken parseName();
  ConfigToken parseValue();
  ConfigToken parseString();
  ConfigToken parseRegEx();

 private:
  std::string label_;
  std::unique_ptr<std::istream> input_;
  ConfigToken currentToken_;
  int currentLineNr_;
  int currentColumn_;
  int currentChar_;
  std::stringstream stringValue_;
};

// {{{ inlines
inline ConfigToken ConfigLexer::currentToken() const noexcept {
  return currentToken_;
}

inline int ConfigLexer::currentLineNr() const noexcept {
  return currentLineNr_;
}

inline int ConfigLexer::currentColumn() const noexcept {
  return currentColumn_;
}

inline int ConfigLexer::currentChar() const noexcept {
  return currentChar_;
}

inline std::string ConfigLexer::stringValue() const {
  return stringValue_.str();
}
// }}}

class ConfigParser {
 public:
  ConfigParser(std::unique_ptr<ConfigLexer> lexer, ProcessConfig* result);

  ReturnCode parse();

 private:
  std::string stringValue() const;
  ConfigToken currentToken() const noexcept;
  ConfigToken nextToken();
  bool eof() const;

  ReturnCode consumeName(std::string* output);
  ReturnCode consumePath(std::string* output);
  ReturnCode consumeValue(std::string* output);
  ReturnCode consumeEndOfLine();
  ReturnCode consumeToken(ConfigToken expected);

  ReturnCode goal();
  ReturnCode pluginDecl();
  ReturnCode eventDecl(EventBindingConfig* output);
  ReturnCode applyEventProperties(const PropertyList& props,
                                  EventBindingConfig* output);
  ReturnCode outputDecl(TargetBindingConfig* output);
  ReturnCode outputProperty();
  ReturnCode propertyList(PropertyList* output);

  ReturnCode unexpectedToken();

 private:
  std::unique_ptr<ConfigLexer> lexer_;
  ProcessConfig* config_;
};

} // namespace evcollect
