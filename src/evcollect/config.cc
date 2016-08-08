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
#include <initializer_list>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <vector>
#include <evcollect/config.h>
#include <evcollect/util/stringutil.h>
#include <evcollect/util/logging.h>

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

ReturnCode loadConfig(
    const std::string& config_file_path,
    ProcessConfig* conf) {
  conf->load_plugins.push_back("./plugins/hostname/.libs/plugin_hostname.so");
  conf->load_plugins.push_back("./plugins/eventql/.libs/plugin_eventql.so");

  {
    // XXX: event sys.alive interval 1s
    conf->event_bindings.emplace_back();
    auto& b = conf->event_bindings.back();
    b.event_name = "sys.alive";
    b.interval_micros = 1000000;

    // XXX: source plugin hostname
    b.sources.emplace_back();
    auto& s = b.sources.back();
    s.plugin_name = "hostname";
  }

  {
    // XXX: event logs.access_log interval 1s
    conf->event_bindings.emplace_back();
    auto& b = conf->event_bindings.back();
    b.event_name = "logs.access_log";
    b.interval_micros = 1000000;
    // XXX: source plugin logfile logfile "/tmp/log" regex /(?<fuu>[^\\|]*)?(?<bar>.*)/
    b.sources.emplace_back();
    auto& s = b.sources.back();
    s.plugin_name = "logfile";
    s.properties.properties.emplace_back(
        std::make_pair(
            "logfile",
            std::vector<std::string> { "/tmp/log" }));

    s.properties.properties.emplace_back(
        std::make_pair(
            "regex",
            std::vector<std::string> { "(?<fuu>[^\\|]*)?(?<bar>.*)" }));
  }

  {
    // XXX: target "eventql1" plugin eventql
    conf->target_bindings.emplace_back();
    auto& b = conf->target_bindings.back();
    b.plugin_name = "eventql";

    // XXX: route logs.access_log "test/logs.access_log"
    b.properties.properties.emplace_back(
        std::make_pair(
            "route",
            std::vector<std::string> { "logs.access_log", "test/logs.access_log" }));

    // XXX: route sys.alive "test/sys.alive"
    b.properties.properties.emplace_back(
        std::make_pair(
            "route",
            std::vector<std::string> { "sys.alive", "test/sys.alive" }));

    // XXX: route sys.alive "test/sys.alive.rollup"
    b.properties.properties.emplace_back(
        std::make_pair(
            "route",
            std::vector<std::string> { "sys.alive", "test/sys.alive.rollup" }));
  }

  return ReturnCode::success();
}

bool PropertyList::get(const std::string& key, std::string* out) const {
  for (const auto& p : properties) {
    if (p.first != key) {
      continue;
    }

    if (p.second.empty()) {
      continue;
    }

    *out = p.second.front();
    return true;
  }

  return false;
}

bool PropertyList::get(const std::string& key, const char** out) const {
  for (const auto& p : properties) {
    if (p.first != key) {
      continue;
    }

    if (p.second.empty()) {
      continue;
    }

    *out = p.second.front().c_str();
    return true;
  }

  return false;
}

bool PropertyList::getv(
    const std::string& key,
    size_t i,
    size_t j,
    const char** out) const {
  for (const auto& p : properties) {
    if (p.first != key) {
      continue;
    }

    if (i > 0) {
      --i;
      continue;
    }

    if (j + 1 > p.second.size()) {
      return false;
    } else {
      *out = p.second[j].c_str();
      return true;
    }
  }

  return false;
}

size_t PropertyList::get(
    const std::string& key,
    std::vector<std::vector<std::string>>* out) const {
  size_t cnt = 0;

  for (const auto& p : properties) {
    if (p.first == key) {
      out->push_back(p.second);
      ++cnt;
    }
  }

  return cnt;
}

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

ConfigParser::ConfigParser(std::unique_ptr<ConfigLexer> lexer, ProcessConfig* result)
  : lexer_(std::move(lexer)),
    config_(result) {
}

ReturnCode ConfigParser::parse() {
  return goal();
}

std::string ConfigParser::stringValue() const {
  return lexer_->stringValue();
}

ConfigToken ConfigParser::currentToken() const noexcept {
  return lexer_->currentToken();
}

ConfigToken ConfigParser::nextToken() {
  return lexer_->nextToken();
}

bool ConfigParser::eof() const {
  return lexer_->eof();
}

// Config          ::= PluginDecl* ( EventDecl | OutputDecl )*
ReturnCode ConfigParser::goal() {
  while (currentToken() == ConfigToken::Plugin) {
    ReturnCode rc = pluginDecl();

    if (rc.isError()) {
      return rc;
    }
  }

  for (;;) {
    switch (currentToken()) {
      case ConfigToken::Event: {
        EventConfig event;
        ReturnCode rc = eventDecl(&event);
        if (rc.isError()) {
          return rc;
        }
        config_->event_bindings.emplace_back(std::move(event));
      }
      case ConfigToken::Output: {
        TargetConfig output;
        ReturnCode rc = outputDecl(&output);
        if (rc.isError()) {
          return rc;
        }
        config_->target_bindings.emplace_back(std::move(output));
      }
      default: {
        return unexpectedToken();
      }
    }
  }

}

ReturnCode ConfigParser::consumeName(std::string* output) {
  switch (currentToken()) {
    case ConfigToken::Name:
      *output = stringValue();
      nextToken();
      return ReturnCode::success();
    default:
      return ReturnCode::error("Unexpected Token", "Expected PATH");
  }
}

ReturnCode ConfigParser::consumePath(std::string* output) {
  switch (currentToken()) {
    case ConfigToken::Name:
    case ConfigToken::Value: // XXX eh! shouldn't be a regex for example, d'oh!
      *output = stringValue();
      nextToken();
      return ReturnCode::success();
    default:
      return ReturnCode::error("Unexpected Token", "Expected PATH");
  }
}

ReturnCode ConfigParser::consumeValue(std::string* output) {
  switch (currentToken()) {
    case ConfigToken::Name:
    case ConfigToken::Value:
      *output = stringValue();
      nextToken();
      return ReturnCode::success();
    default:
      return ReturnCode::error("Unexpected Token", "Expected PATH");
  }
}

ReturnCode ConfigParser::consumeEndOfLine() {
  if (currentToken() == ConfigToken::End) {
    nextToken();
    return ReturnCode::success();
  }

  return ReturnCode::error("Unexpected Token", "Expected end of line.");
}

// PluginDecl      ::= "plugin" PATH NL
ReturnCode ConfigParser::pluginDecl() {
  nextToken();

  std::string path;
  ReturnCode rc = consumePath(&path);
  if (rc.isError()) {
    return rc;
  }

  rc = consumeEndOfLine();
  if (rc.isError()) {
    return rc;
  }

  // this is basename or a path
  config_->load_plugins.emplace_back(path);

  return ReturnCode::success();
}

// EventDecl       ::= "event" NAME PropertyList NL EventSourceDecl*
// EventSourceDecl ::= "source" NAME VALUE PropertyList NL
ReturnCode ConfigParser::eventDecl(EventConfig* event) {
  nextToken(); // skip "event"

  ReturnCode rc = consumeName(&event->event_name);
  if (rc.isError())
    return rc;

  PropertyList eventProperties;
  rc = propertyList(&eventProperties);
  if (rc.isError())
    return rc;

  rc = applyEventProperties(eventProperties, event);
  if (rc.isError())
    return rc;

  rc = consumeEndOfLine();
  if (rc.isError())
    return rc;

  while (currentToken() == ConfigToken::Source) {
    nextToken();

    EventSourceConfig source;
    rc = consumeName(&source.plugin_name);
    if (rc.isError())
      return rc;

    rc = consumeValue(&source.plugin_value);
    if (rc.isError())
      return rc;

    rc = propertyList(&source.properties);
    if (rc.isError())
      return rc;

    event->sources.emplace_back(std::move(source));
  }

  return ReturnCode::success();
}

ReturnCode ConfigParser::applyEventProperties(const PropertyList& props,
                                              EventConfig* output) {

  for (const auto& prop: props.properties) {
    if (prop.first == "interval") {
      output->interval_micros = 1000000; // TODO: parseTime(prop.second[0]) FIXME not sure if that's meant like this
    } else {
      logWarning("Ignoring unsupported property \"%s\".", prop.first);
    }
  }

  return ReturnCode::success();
}

// OutputDecl      ::= "output" NAME "plugin" PATH NL OutputProperty*
ReturnCode ConfigParser::outputDecl(TargetConfig* output) {
  nextToken(); // skip "output"

  ReturnCode rc = consumeName(&output->plugin_value);
  if (rc.isError())
    return rc;

  rc = consumeToken(ConfigToken::Plugin);
  if (rc.isError())
    return rc;

  rc = consumePath(&output->plugin_name);
  if (rc.isError())
    return rc;

  rc = consumeEndOfLine();
  if (rc.isError())
    return rc;

  return ReturnCode::success();
}

// OutputProperty  ::= OutputKey PropertyList NL
// OutputKey       ::= NAME
ReturnCode ConfigParser::outputProperty() {
  // TODO
  return ReturnCode::success();
}

// PropertyList    ::= (NAME VALUE)*
ReturnCode ConfigParser::propertyList(PropertyList* output) {
  while (currentToken() == ConfigToken::Name) {
    std::string name = stringValue();

    std::string value;
    ReturnCode rc = consumeValue(&value);
    if (rc.isError())
      return rc;

    std::vector<std::string> values = { value };

    output->properties.emplace_back(std::move(name), std::move(values));
  }

  return ReturnCode::success();
}

ReturnCode ConfigParser::unexpectedToken() {
  return ReturnCode::error("Unepxected Token",
                           "<%s>: Expected \"event\" or \"output\", but got %s.",
                           lexer_->currentContext().c_str(),
                           StringUtil::toString(lexer_->currentToken()).c_str());
}

} // namespace evcollect
