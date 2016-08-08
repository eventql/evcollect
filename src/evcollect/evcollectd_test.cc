#include <evcollect/util/testing.h>

TEST(ConfigLexer, empty) {
  auto lexer = ConfigLexer::fromString("");

  EXPECT_EQ(ConfigToken::Eof, lexer->currentToken());
  EXPECT_EQ(1, lexer->currentLineNr());
  EXPECT_EQ(1, lexer->currentColumn());
}

TEST(ConfigLexer, keywords) {
  auto lexer = ConfigLexer::fromString("event source output plugin SomeThing");

  EXPECT_EQ(ConfigToken::Event, lexer->currentToken());
  ASSERT_EQ("event", lexer->stringValue());

  lexer->nextToken();
  ASSERT_EQ(ConfigToken::Source, lexer->currentToken());
  ASSERT_EQ("source", lexer->stringValue());

  lexer->nextToken();
  ASSERT_EQ(ConfigToken::Output, lexer->currentToken());
  ASSERT_EQ("output", lexer->stringValue());

  lexer->nextToken();
  ASSERT_EQ(ConfigToken::Plugin, lexer->currentToken());
  ASSERT_EQ("plugin", lexer->stringValue());

  lexer->nextToken();
  ASSERT_EQ(ConfigToken::Name, lexer->currentToken());
  ASSERT_EQ("SomeThing", lexer->stringValue());

  lexer->nextToken();
  ASSERT_EQ(ConfigToken::Eof, lexer->currentToken());
}

TEST(ConfigLexer, newlines) {
  auto lexer = ConfigLexer::fromString("event\nsource\n\noutput");

  EXPECT_EQ(ConfigToken::Event, lexer->currentToken());
  ASSERT_EQ("event", lexer->stringValue());

  lexer->nextToken();
  ASSERT_EQ(ConfigToken::End, lexer->currentToken());
  ASSERT_EQ("LF", lexer->stringValue());

  lexer->nextToken();
  ASSERT_EQ(ConfigToken::Source, lexer->currentToken());
  ASSERT_EQ("source", lexer->stringValue());

  lexer->nextToken();
  ASSERT_EQ(ConfigToken::End, lexer->currentToken());
  ASSERT_EQ("LF", lexer->stringValue());

  lexer->nextToken();
  ASSERT_EQ(ConfigToken::Output, lexer->currentToken());
  ASSERT_EQ("output", lexer->stringValue());

  lexer->nextToken();
  ASSERT_EQ(ConfigToken::Eof, lexer->currentToken());
}

TEST(ConfigLexer, eventDecl_example1) {
  auto lexer = ConfigLexer::fromString("event sys.alive interval 1s\n");

  ASSERT_EQ(ConfigToken::Event, lexer->currentToken());
  ASSERT_EQ("event", lexer->stringValue());

  lexer->nextToken();
  ASSERT_EQ(ConfigToken::Name, lexer->currentToken());
  ASSERT_EQ("sys.alive", lexer->stringValue());

  lexer->nextToken();
  ASSERT_EQ(ConfigToken::Name, lexer->currentToken());
  ASSERT_EQ("interval", lexer->stringValue());

  lexer->nextToken();
  ASSERT_EQ(ConfigToken::Value, lexer->currentToken());
  ASSERT_EQ("1s", lexer->stringValue());

  lexer->nextToken();
  ASSERT_EQ(ConfigToken::End, lexer->currentToken());
  ASSERT_EQ("LF", lexer->stringValue());
}

TEST(ConfigParser, Blurb) {
  logf("Blurbed $0", "!");
  ASSERT_EQ(2, 1 + 1);
}
