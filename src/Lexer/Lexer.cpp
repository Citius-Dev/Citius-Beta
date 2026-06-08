#include "Lexer/Lexer.h"
#include "Basic/TokenKinds.h"
#include <cctype>
#include <cstdlib>

const std::unordered_map<std::string, TokenKind> Lexer::Keywords = {
    // Deprecated compat
    {"fn", TokenKind::KwFn},
    {"let", TokenKind::KwLet},
    {"var", TokenKind::KwVar},
    // Core keywords
    {"func", TokenKind::KwFunc},
    {"if", TokenKind::KwIf},
    {"else", TokenKind::KwElse},
    {"while", TokenKind::KwWhile},
    {"for", TokenKind::KwFor},
    {"return", TokenKind::KwReturn},
    {"break", TokenKind::KwBreak},
    {"continue", TokenKind::KwContinue},
    {"true", TokenKind::KwTrue},
    {"false", TokenKind::KwFalse},
    {"void", TokenKind::KwVoid},
    {"bool", TokenKind::KwBool},
    {"i8", TokenKind::KwI8},
    {"i16", TokenKind::KwI16},
    {"i32", TokenKind::KwI32},
    {"i64", TokenKind::KwI64},
    {"u8", TokenKind::KwU8},
    {"u16", TokenKind::KwU16},
    {"u32", TokenKind::KwU32},
    {"u64", TokenKind::KwU64},
    {"f32", TokenKind::KwF32},
    {"f64", TokenKind::KwF64},
    {"struct", TokenKind::KwStruct},
    {"import", TokenKind::KwImport},
    {"pub", TokenKind::KwPub},
    {"mut", TokenKind::KwMut},
    {"in", TokenKind::KwIn},
    // New language features
    {"otherhave", TokenKind::KwOtherhave},
    {"annotate", TokenKind::KwAnnotate},
    {"package", TokenKind::KwPackage},
    {"sleep", TokenKind::KwSleep},
    {"wait", TokenKind::KwWait},
    {"sent", TokenKind::KwSent},
    {"headers", TokenKind::KwHeaders},
    {"payload", TokenKind::KwPayload},
    {"raw", TokenKind::KwRaw},
    {"hex", TokenKind::KwHex},
    {"binary", TokenKind::KwBinary},
    {"text", TokenKind::KwText},
    // Time units
    {"ms", TokenKind::KwMs},
    {"s", TokenKind::KwSec},
    {"min", TokenKind::KwMin},
    {"h", TokenKind::KwHour},
    {"d", TokenKind::KwDay},
    {"m", TokenKind::KwMonth},
    {"y", TokenKind::KwYear},
    // Network protocols
    {"http", TokenKind::KwHttp},
    {"https", TokenKind::KwHttps},
    {"tcp", TokenKind::KwTcp},
    {"udp", TokenKind::KwUdp},
    // Sent modes
    {"auto", TokenKind::KwAuto},
    {"manual", TokenKind::KwManual},
};

char Lexer::peek(size_t Ahead) const {
  size_t Idx = Pos + Ahead;
  if (Idx >= Source.size())
    return '\0';
  return Source[Idx];
}

char Lexer::advance() {
  if (Pos >= Source.size())
    return '\0';
  char C = Source[Pos++];
  if (C == '\n') {
    Line++;
    Col = 1;
  } else {
    Col++;
  }
  return C;
}

void Lexer::skipWhitespaceAndComments() {
  while (Pos < Source.size()) {
    char C = peek();
    if (C == ' ' || C == '\t' || C == '\n' || C == '\r') {
      advance();
    } else if (C == '/' && peek(1) == '/') {
      // Line comment //
      while (Pos < Source.size() && peek() != '\n')
        advance();
    } else if (C == '/' && peek(1) == '*') {
      // Block comment /* */
      advance();
      advance();
      while (Pos < Source.size() && !(peek() == '*' && peek(1) == '/'))
        advance();
      if (Pos < Source.size())
        advance();
      if (Pos < Source.size())
        advance();
    } else if (C == '#') {
      // Line comment #
      while (Pos < Source.size() && peek() != '\n')
        advance();
    } else if (C == '<' && peek(1) == '!' && peek(2) == '-' &&
               peek(3) == '-' && peek(4) == '-') {
      // Block comment <!--- --->
      advance(); advance(); advance(); advance(); advance(); // <!---
      while (Pos < Source.size() && !(peek() == '-' && peek(1) == '-' &&
                                      peek(2) == '-' && peek(3) == '>'))
        advance();
      if (Pos < Source.size())
        advance(); // -
      if (Pos < Source.size())
        advance(); // -
      if (Pos < Source.size())
        advance(); // -
      if (Pos < Source.size())
        advance(); // >
    } else {
      break;
    }
  }
}

Token Lexer::lexIdentifierOrKeyword() {
  auto Loc = curLoc();
  size_t Start = Pos;
  while (std::isalnum(peek()) || peek() == '_')
    advance();
  std::string Text = Source.substr(Start, Pos - Start);
  auto It = Keywords.find(Text);
  TokenKind Kind = (It != Keywords.end()) ? It->second : TokenKind::Identifier;
  return Token(Kind, Loc, std::move(Text));
}

Token Lexer::lexNumber() {
  auto Loc = curLoc();
  size_t Start = Pos;
  bool IsFloat = false;

  while (std::isdigit(peek()))
    advance();

  if (peek() == '.' && std::isdigit(peek(1))) {
    IsFloat = true;
    advance();
    while (std::isdigit(peek()))
      advance();
  }

  if (peek() == 'e' || peek() == 'E') {
    IsFloat = true;
    advance();
    if (peek() == '+' || peek() == '-')
      advance();
    while (std::isdigit(peek()))
      advance();
  }

  std::string Text = Source.substr(Start, Pos - Start);
  TokenKind Kind = IsFloat ? TokenKind::FloatLiteral : TokenKind::IntegerLiteral;
  return Token(Kind, Loc, std::move(Text));
}

Token Lexer::lexString() {
  auto Loc = curLoc();
  advance();
  size_t Start = Pos;
  std::string Val;

  while (Pos < Source.size() && peek() != '"') {
    if (peek() == '\\') {
      advance();
      switch (peek()) {
      case 'n':
        Val += '\n';
        break;
      case 't':
        Val += '\t';
        break;
      case '\\':
        Val += '\\';
        break;
      case '"':
        Val += '"';
        break;
      case '0':
        Val += '\0';
        break;
      default:
        Val += peek();
        break;
      }
      advance();
    } else {
      Val += advance();
    }
  }

  if (Pos >= Source.size()) {
    Diags.error(Loc, "unterminated string literal");
    return Token(TokenKind::StringLiteral, Loc, std::move(Val));
  }

  advance();
  return Token(TokenKind::StringLiteral, Loc, std::move(Val));
}

Token Lexer::lexChar() {
  auto Loc = curLoc();
  advance();
  char Val = '\0';

  if (peek() == '\\') {
    advance();
    switch (peek()) {
    case 'n':
      Val = '\n';
      break;
    case 't':
      Val = '\t';
      break;
    case '\\':
      Val = '\\';
      break;
    case '\'':
      Val = '\'';
      break;
    case '0':
      Val = '\0';
      break;
    default:
      Val = peek();
      break;
    }
    advance();
  } else {
    Val = advance();
  }

  if (peek() != '\'') {
    Diags.error(Loc, "unterminated char literal");
  } else {
    advance();
  }

  return Token(TokenKind::CharLiteral, Loc, std::string(1, Val));
}

Token Lexer::lexPunctuation() {
  auto Loc = curLoc();
  char C = advance();

  switch (C) {
  case '+':
    if (peek() == '=')
      return advance(), Token(TokenKind::PlusEq, Loc, "+=");
    return Token(TokenKind::Plus, Loc, "+");
  case '-':
    if (peek() == '=')
      return advance(), Token(TokenKind::MinusEq, Loc, "-=");
    if (peek() == '>')
      return advance(), Token(TokenKind::Arrow, Loc, "->");
    return Token(TokenKind::Minus, Loc, "-");
  case '*':
    if (peek() == '=')
      return advance(), Token(TokenKind::StarEq, Loc, "*=");
    return Token(TokenKind::Star, Loc, "*");
  case '/':
    if (peek() == '=')
      return advance(), Token(TokenKind::SlashEq, Loc, "/=");
    return Token(TokenKind::Slash, Loc, "/");
  case '%':
    if (peek() == '=')
      return advance(), Token(TokenKind::PercentEq, Loc, "%=");
    return Token(TokenKind::Percent, Loc, "%");
  case '&':
    if (peek() == '&')
      return advance(), Token(TokenKind::AmpAmp, Loc, "&&");
    return Token(TokenKind::Amp, Loc, "&");
  case '|':
    if (peek() == '|')
      return advance(), Token(TokenKind::PipePipe, Loc, "||");
    return Token(TokenKind::Pipe, Loc, "|");
  case '^':
    return Token(TokenKind::Caret, Loc, "^");
  case '~':
    return Token(TokenKind::Tilde, Loc, "~");
  case '!':
    if (peek() == '=')
      return advance(), Token(TokenKind::ExclaimEq, Loc, "!=");
    return Token(TokenKind::Exclaim, Loc, "!");
  case '=':
    if (peek() == '=')
      return advance(), Token(TokenKind::EqEq, Loc, "==");
    return Token(TokenKind::Eq, Loc, "=");
  case '<':
    if (peek() == '=')
      return advance(), Token(TokenKind::LessEq, Loc, "<=");
    if (peek() == '<')
      return advance(), Token(TokenKind::LessLess, Loc, "<<");
    return Token(TokenKind::Less, Loc, "<");
  case '>':
    if (peek() == '=')
      return advance(), Token(TokenKind::GreaterEq, Loc, ">=");
    if (peek() == '>')
      return advance(), Token(TokenKind::GreaterGreater, Loc, ">>");
    return Token(TokenKind::Greater, Loc, ">");
  case '.':
    if (peek() == '.')
      return advance(), Token(TokenKind::DotDot, Loc, "..");
    return Token(TokenKind::Dot, Loc, ".");
  case ':':
    return Token(TokenKind::Colon, Loc, ":");
  case ';':
    return Token(TokenKind::Semi, Loc, ";");
  case ',':
    return Token(TokenKind::Comma, Loc, ",");
  case '(':
    return Token(TokenKind::LParen, Loc, "(");
  case ')':
    return Token(TokenKind::RParen, Loc, ")");
  case '{':
    return Token(TokenKind::LBrace, Loc, "{");
  case '}':
    return Token(TokenKind::RBrace, Loc, "}");
  case '[':
    return Token(TokenKind::LBracket, Loc, "[");
  case ']':
    return Token(TokenKind::RBracket, Loc, "]");
  default:
    Diags.error(Loc, std::string("unexpected character '") + C + "'");
    return Token(TokenKind::Unknown, Loc, std::string(1, C));
  }
}

Token Lexer::nextToken() {
  if (HasPeeked) {
    HasPeeked = false;
    return LastToken;
  }

  skipWhitespaceAndComments();

  if (Pos >= Source.size())
    return Token(TokenKind::Eof, curLoc(), "");

  char C = peek();
  if (std::isalpha(C) || C == '_')
    return lexIdentifierOrKeyword();
  if (std::isdigit(C))
    return lexNumber();
  if (C == '"')
    return lexString();
  if (C == '\'')
    return lexChar();
  return lexPunctuation();
}

Token Lexer::peekToken() {
  if (!HasPeeked) {
    LastToken = nextToken();
    HasPeeked = true;
  }
  return LastToken;
}
