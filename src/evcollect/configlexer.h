#pragma once

#include <memory>
#include <istream>
#include <sstream>

namespace evcollect {

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

} // namespace evcollect
