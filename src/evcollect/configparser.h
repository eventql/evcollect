#pragma once

#include <evcollect/config.h> // PropertyList
#include <evcollect/configlexer.h>
#include <evcollect/util/return_code.h>
#include <memory>

namespace evcollect {

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
