#ifndef CITIUS_TOKEN_KINDS_H
#define CITIUS_TOKEN_KINDS_H

enum class TokenKind {
  // End of file / error
  Eof,
  Unknown,

  // Identifiers and literals
  Identifier,
  IntegerLiteral,
  FloatLiteral,
  StringLiteral,
  CharLiteral,

  // Keywords
  KwFn,         // deprecated, kept for compat
  KwLet,        // deprecated, kept for compat
  KwVar,        // deprecated, kept for compat
  KwFunc,       // function definition
  KwIf,
  KwElse,
  KwWhile,
  KwFor,
  KwReturn,
  KwBreak,
  KwContinue,
  KwTrue,
  KwFalse,
  KwVoid,
  KwBool,
  KwI8, KwI16, KwI32, KwI64,
  KwU8, KwU16, KwU32, KwU64,
  KwF32, KwF64,
  KwStruct,
  KwImport,
  KwPub,
  KwMut,
  KwIn,

  // New language features
  KwOtherhave,  // otherhave declaration
  KwAnnotate,   // annotate block
  KwPackage,    // package declaration (replaces packet)
  KwSleep,      // sleep delay
  KwWait,       // wait delay
  KwSent,       // network request
  KwHeaders,    // headers block in sent
  KwPayload,    // payload in sent
  KwRaw,        // raw mode flag in sent
  KwHex,        // hex data type (manual sent)
  KwBinary,     // binary data type (manual sent)
  KwText,       // text data type (manual sent)

  // Time units (used in sleep/wait)
  KwMs,         // milliseconds
  KwSec,        // seconds
  KwMin,        // minutes
  KwHour,       // hours
  KwDay,        // days
  KwMonth,      // months (30 days)
  KwYear,       // years (365 days)

  // Network protocols
  KwHttp,
  KwHttps,
  KwTcp,
  KwUdp,

  // Sent modes
  KwAuto,
  KwManual,

  // Punctuation
  Plus,           // +
  Minus,          // -
  Star,           // *
  Slash,          // /
  Percent,        // %
  Amp,            // &
  Pipe,           // |
  Caret,          // ^
  Tilde,          // ~
  Exclaim,        // !

  PlusEq,         // +=
  MinusEq,        // -=
  StarEq,         // *=
  SlashEq,        // /=
  PercentEq,      // %=
  AmpAmp,         // &&
  PipePipe,       // ||
  EqEq,           // ==
  ExclaimEq,      // !=
  Less,           // <
  Greater,        // >
  LessEq,         // <=
  GreaterEq,      // >=
  LessLess,       // <<
  GreaterGreater, // >>
  Arrow,          // ->
  Dot,            // .
  DotDot,         // ..

  Eq,             // =
  Semi,           // ;
  Colon,          // :
  Comma,          // ,
  LParen,         // (
  RParen,         // )
  LBrace,         // {
  RBrace,         // }
  LBracket,       // [
  RBracket,       // ]
};

#endif
