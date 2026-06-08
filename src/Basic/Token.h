#ifndef CITIUS_TOKEN_H
#define CITIUS_TOKEN_H

#include "TokenKinds.h"
#include "SourceLocation.h"
#include <string>
#include <string_view>

class Token {
public:
  TokenKind Kind{TokenKind::Unknown};
  SourceLocation Loc;
  std::string Text;

  Token() = default;

  Token(TokenKind Kind, SourceLocation Loc, std::string Text = {})
      : Kind(Kind), Loc(Loc), Text(std::move(Text)) {}

  bool is(TokenKind K) const { return Kind == K; }
  bool isNot(TokenKind K) const { return Kind != K; }
  bool isOneOf(TokenKind K1, TokenKind K2) const {
    return is(K1) || is(K2);
  }
  template <typename... Ts>
  bool isOneOf(TokenKind K1, TokenKind K2, Ts... Ks) const {
    return is(K1) || isOneOf(K2, Ks...);
  }

  std::string_view getText() const { return Text; }
  double getFloatValue() const;
  int64_t getIntValue() const;

  static const char *getTokenName(TokenKind K);
};

#endif
