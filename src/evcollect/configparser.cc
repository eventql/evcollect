#include <evcollect/configparser.h>
#include <evcollect/util/logging.h>
#include <evcollect/util/stringutil.h>
#include <vector>

namespace evcollect {

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
        EventBindingConfig event;
        ReturnCode rc = eventDecl(&event);
        if (rc.isError()) {
          return rc;
        }
        config_->event_bindings.emplace_back(std::move(event));
      }
      case ConfigToken::Output: {
        TargetBindingConfig output;
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
ReturnCode ConfigParser::eventDecl(EventBindingConfig* event) {
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

    EventSourceBindingConfig source;
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
                                              EventBindingConfig* output) {

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
ReturnCode ConfigParser::outputDecl(TargetBindingConfig* output) {
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
