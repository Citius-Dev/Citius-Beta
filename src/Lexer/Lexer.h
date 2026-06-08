#ifndef CITIUS_LEXER_H
#define CITIUS_LEXER_H

#include "Basic/Token.h"
#include "Basic/Diagnostic.h"
#include <string>
#include <string_view>
#include <unordered_map>

class Lexer {
public:
  Lexer(DiagnosticsEngine &Diags, std::string Filename, std::string Source)
      : Diags(Diags), Filename(std::move(Filename)),
        Source(std::move(Source)), Pos(0) {}

  Token nextToken();
  Token peekToken();
  void consume() { nextToken(); }

  void setSource(std::string Source) {
    this->Source = std::move(Source);
    Pos = 0;
  }

private:
  DiagnosticsEngine &Diags;
  std::string Filename;
  std::string Source;
  size_t Pos;
  Token LastToken;
  bool HasPeeked{false};

  uint32_t Line{1};
  uint32_t Col{1};

  char peek(size_t Ahead = 0) const;
  char advance();
  void skipWhitespaceAndComments();
  Token lexIdentifierOrKeyword();
  Token lexNumber();
  Token lexString();
  Token lexChar();
  Token lexPunctuation();

  SourceLocation curLoc() const {
    return SourceLocation(Line, Col, static_cast<uint32_t>(Pos), Filename);
  }

  static const std::unordered_map<std::string, TokenKind> Keywords;
};

#endif
