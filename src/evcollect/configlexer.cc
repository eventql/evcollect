#include <evcollect/configlexer.h>
#include <evcollect/util/stringutil.h>
#include <initializer_list>
#include <unordered_map>
#include <sstream>
#include <fstream>

template<>
std::string StringUtil::toString(evcollect::ConfigToken value) {
  switch (value) {
    case evcollect::ConfigToken::Unknown: return "<UNKNOWN>";
    case evcollect::ConfigToken::Eof: return "<EOF>";
    case evcollect::ConfigToken::End: return "<END>";
    case evcollect::ConfigToken::Name: return "<NAME>";
    case evcollect::ConfigToken::Value: return "<VALUE>";
    case evcollect::ConfigToken::Event: return "event";
    case evcollect::ConfigToken::Source: return "source";
    case evcollect::ConfigToken::Output: return "output";
    case evcollect::ConfigToken::Plugin: return "plugin";
    // no default to get compiler warning, so we know we've fsck'd
  }
}

namespace evcollect {

static const int CR = '\r';
static const int LF = '\n';

ConfigLexer::ConfigLexer(const std::string& label,
                         std::unique_ptr<std::istream>&& input)
    : label_(label),
      input_(std::move(input)),
      currentLineNr_(1),
      currentColumn_(0),
      currentChar_('\0'),
      stringValue_() {
  printf("ConfigLexer()\n");
  nextToken();
}

std::unique_ptr<ConfigLexer> ConfigLexer::fromString(const std::string& config) {
  return std::unique_ptr<ConfigLexer>(new ConfigLexer(
        "<string>",
        std::unique_ptr<std::istream>(new std::stringstream(config))));
}

std::unique_ptr<ConfigLexer> ConfigLexer::fromLocalFile(const std::string& path) {
  return std::unique_ptr<ConfigLexer>(new ConfigLexer(
        path,
        std::unique_ptr<std::istream>(new std::ifstream(path))));
}

std::string ConfigLexer::currentContext() const {
  return StringUtil::format("$0:$1:$2", label_, currentLineNr_, currentColumn_);
}

ConfigToken ConfigLexer::nextToken() {
  // reset stringValue_
  stringValue_.str(std::string());
  stringValue_.clear();

  printf("ConfigLexer.nextToken() <%s>: 0x%02x %c\n",
         currentContext().c_str(),
         currentChar_,
         std::isalpha(currentChar_) ? currentChar_ : 0);

  if (!consumeSpace()) {
    return currentToken_ = ConfigToken::Eof;
  }

  if (consumeEndOfLine()) {
    return currentToken_ = ConfigToken::End;
  }

  if (currentChar() == '"' || currentChar() == '\'') {
    return currentToken_ = parseString();
  }

  if (currentChar() == '/') {
    return currentToken_ = parseRegEx();
  }

  if (std::isalpha(currentChar())) {
    return currentToken_ = parseName();
  }

  if (std::isdigit(currentChar())) {
    return currentToken_ = parseValue();
  }

  return currentToken_ = ConfigToken::Unknown; 
}

bool ConfigLexer::consumeEndOfLine() {
  bool found = false;

  while (!eof()) {
    switch (currentChar()) {
      case LF:
        nextChar(); // skip LF
        found = true;
        break;
      case CR:
        if (peekChar() == LF) {
          nextChar(); // skip CR
          nextChar(); // skip LF
          found = true;
          break;
        }
      default:
        goto done;
    }
  }

done:
  if (found) {
    stringValue_ << "LF";
  }
  return found;
}

ConfigToken ConfigLexer::parseValue() {
  // shall the first character be assured.

  do {
    stringValue_ << (char) currentChar();
    nextChar();
  } while (std::isalnum(currentChar()) || currentChar() == '_' ||
           currentChar() == '.' || currentChar() == ':');

  return ConfigToken::Value;
}

ConfigToken ConfigLexer::parseString() {
  int delim = currentChar();
  int last = -1;

  nextChar();  // skip left delimiter

  while (!eof() && (currentChar() != delim || (last == '\\'))) {
    stringValue_ << static_cast<char>(currentChar());

    last = currentChar();
    nextChar();
  }

  if (currentChar() == delim) {
    nextChar();

    return ConfigToken::Value;
  }

  return ConfigToken::Unknown;
}

ConfigToken ConfigLexer::parseRegEx() {
  return parseString(); // not kidding! same!
}

ConfigToken ConfigLexer::parseName() {
  // shall the first character be assured.

  do {
    stringValue_ << (char) currentChar();
    nextChar();
  } while (std::isalnum(currentChar()) || currentChar() == '_' ||
           currentChar() == '.' || currentChar() == ':');

  static std::unordered_map<std::string, ConfigToken> keywords = {
    {"event", ConfigToken::Event},
    {"source", ConfigToken::Source},
    {"output", ConfigToken::Output},
    {"plugin", ConfigToken::Plugin},
  };

  auto i = keywords.find(stringValue_.str());
  if (i != keywords.end()) {
    return i->second;
  }

  return ConfigToken::Name;
}

/**
 * @retval true data pending
 * @retval false EOF reached
 */
bool ConfigLexer::consumeSpace() {
  // skip spaces
  for (;; nextChar()) {
    if (eof())
      return false;

    if (currentChar_ == CR || currentChar_ == LF)
      return true;

    if (std::isspace(currentChar_))
      continue;

    if (std::isprint(currentChar_))
      break;
  }

  if (eof())
    return false;

  if (currentChar() == '#') {
    nextChar();

    // skip chars until EOL
    for (;;) {
      if (eof()) {
        currentToken_ = ConfigToken::Eof;
        return false;
      }

      if (currentChar() == CR || currentChar() == LF) {
        return true;
      }

      nextChar();
    }
  }

  if (currentChar() == '/' && peekChar() == '*') {  // "/*" ... "*/"
    // parse multiline comment
    nextChar();

    for (;;) {
      if (eof()) {
        currentToken_ = ConfigToken::Eof;
        // reportError(Error::UnexpectedEof);
        return false;
      }

      if (currentChar() == '*' && peekChar() == '/') {
        nextChar();  // skip '*'
        nextChar();  // skip '/'
        break;
      }

      nextChar();
    }

    return consumeSpace();
  }

  return true;
}

int ConfigLexer::peekChar() {
  return input_->peek();
}

bool ConfigLexer::eof() const {
  return currentChar_ == EOF || input_->eof();
}

int ConfigLexer::nextChar() {
  if (currentChar_ == EOF) {
    return EOF;
  }

  currentChar_ = input_->get();

  if (currentChar_ != EOF) {
    if (currentChar_ != '\n') {
      currentColumn_++;
    } else {
      currentLineNr_++;
      currentColumn_ = 1;
    }
    printf("ConfigLexer.nextChar() <%s>: 0x%02x %c\n",
           currentContext().c_str(),
           currentChar_,
           std::isalpha(currentChar_) ? currentChar_ : 0);
  } else {
    currentColumn_++;
    printf("ConfigLexer.nextChar() <%s>: EOF\n",
           currentContext().c_str());
  }

  return currentChar_;
}

} // namespace evcollect
