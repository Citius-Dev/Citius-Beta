#include "Basic/Token.h"
#include <cstdlib>

const char *Token::getTokenName(TokenKind K) {
  switch (K) {
  case TokenKind::Eof:
    return "end of file";
  case TokenKind::Unknown:
    return "unknown";
  case TokenKind::Identifier:
    return "identifier";
  case TokenKind::IntegerLiteral:
    return "integer literal";
  case TokenKind::FloatLiteral:
    return "float literal";
  case TokenKind::StringLiteral:
    return "string literal";
  case TokenKind::CharLiteral:
    return "char literal";
  case TokenKind::KwFn:
    return "fn";
  case TokenKind::KwLet:
    return "let";
  case TokenKind::KwVar:
    return "var";
  case TokenKind::KwIf:
    return "if";
  case TokenKind::KwElse:
    return "else";
  case TokenKind::KwWhile:
    return "while";
  case TokenKind::KwFor:
    return "for";
  case TokenKind::KwReturn:
    return "return";
  case TokenKind::KwBreak:
    return "break";
  case TokenKind::KwContinue:
    return "continue";
  case TokenKind::KwTrue:
    return "true";
  case TokenKind::KwFalse:
    return "false";
  case TokenKind::KwVoid:
    return "void";
  case TokenKind::KwBool:
    return "bool";
  case TokenKind::KwI8:
    return "i8";
  case TokenKind::KwI16:
    return "i16";
  case TokenKind::KwI32:
    return "i32";
  case TokenKind::KwI64:
    return "i64";
  case TokenKind::KwU8:
    return "u8";
  case TokenKind::KwU16:
    return "u16";
  case TokenKind::KwU32:
    return "u32";
  case TokenKind::KwU64:
    return "u64";
  case TokenKind::KwF32:
    return "f32";
  case TokenKind::KwF64:
    return "f64";
  case TokenKind::KwStruct:
    return "struct";
  case TokenKind::KwImport:
    return "import";
  case TokenKind::KwPub:
    return "pub";
  case TokenKind::KwMut:
    return "mut";
  case TokenKind::KwIn:
    return "in";
  case TokenKind::Plus:
    return "+";
  case TokenKind::Minus:
    return "-";
  case TokenKind::Star:
    return "*";
  case TokenKind::Slash:
    return "/";
  case TokenKind::Percent:
    return "%";
  case TokenKind::Amp:
    return "&";
  case TokenKind::Pipe:
    return "|";
  case TokenKind::Caret:
    return "^";
  case TokenKind::Tilde:
    return "~";
  case TokenKind::Exclaim:
    return "!";
  case TokenKind::PlusEq:
    return "+=";
  case TokenKind::MinusEq:
    return "-=";
  case TokenKind::StarEq:
    return "*=";
  case TokenKind::SlashEq:
    return "/=";
  case TokenKind::PercentEq:
    return "%=";
  case TokenKind::AmpAmp:
    return "&&";
  case TokenKind::PipePipe:
    return "||";
  case TokenKind::EqEq:
    return "==";
  case TokenKind::ExclaimEq:
    return "!=";
  case TokenKind::Less:
    return "<";
  case TokenKind::Greater:
    return ">";
  case TokenKind::LessEq:
    return "<=";
  case TokenKind::GreaterEq:
    return ">=";
  case TokenKind::LessLess:
    return "<<";
  case TokenKind::GreaterGreater:
    return ">>";
  case TokenKind::Arrow:
    return "->";
  case TokenKind::Dot:
    return ".";
  case TokenKind::DotDot:
    return "..";
  case TokenKind::Eq:
    return "=";
  case TokenKind::Semi:
    return ";";
  case TokenKind::Colon:
    return ":";
  case TokenKind::Comma:
    return ",";
  case TokenKind::LParen:
    return "(";
  case TokenKind::RParen:
    return ")";
  case TokenKind::LBrace:
    return "{";
  case TokenKind::RBrace:
    return "}";
  case TokenKind::LBracket:
    return "[";
  case TokenKind::RBracket:
    return "]";
  }
  return "unknown";
}

double Token::getFloatValue() const {
  return std::stod(Text);
}

int64_t Token::getIntValue() const {
  return std::stoll(Text);
}
