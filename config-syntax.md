# evcollect syntax specification

## Syntactical Grammar
```
Config          ::= PluginDecl* ( EventDecl | OutputDecl )*

PluginDecl      ::= "plugin" PATH NL

EventDecl       ::= "event" NAME PropertyList NL EventSourceDecl*
EventSourceDecl ::= "source" NAME VALUE PropertyList NL

OutputDecl      ::= "output" NAME "plugin" PATH NL OutputProperty*
OutputProperty  ::= OutputKey PropertyList NL
OutputKey       ::= NAME

PropertyList    ::= (NAME VALUE)*
```

### Lexical Grammar
```
NL              ::= (CR LF | LF)+
CR              ::= 0x0D
LF              ::= 0x0A
NAME            ::= [a-zA-Z] [a-zA-Z0-9_\.:]*
VALUE           ::= NAME | QuotedValue | RegexValue
PATH            ::= NAME | QuotedValue

QuotedValue     ::= '"' [^"]* '"'
RegexValue      ::= '/' RegEx '/'
RegEx            ::= <plain regex suitable for libpcre>
```
