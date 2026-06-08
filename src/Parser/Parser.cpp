#include "Parser/Parser.h"
#include <cctype>

//===----------------------------------------------------------------------===//
// Helper: consume newlines between statements
//===----------------------------------------------------------------------===//

static void skipNewlines(Lexer &Lex) {
  while (Lex.peekToken().isOneOf(TokenKind::Semi, TokenKind::Unknown)) {
    if (Lex.peekToken().is(TokenKind::Unknown)) {
      auto T = Lex.peekToken();
      if (T.getText() == "\n" || T.getText() == "\r") {
        Lex.nextToken();
        continue;
      }
    }
    break;
  }
}

static void stmtSep(Lexer &Lex) {
  if (Lex.peekToken().is(TokenKind::Semi)) {
    Lex.nextToken();
  }
}

//===----------------------------------------------------------------------===//
// Expect helpers
//===----------------------------------------------------------------------===//

bool Parser::expect(TokenKind K) {
  if (curTok().is(K))
    return true;
  Diags.error(curTok().Loc, std::string("expected ") +
                                Token::getTokenName(K) + " but got " +
                                Token::getTokenName(curTok().Kind));
  return false;
}

bool Parser::expectAndConsume(TokenKind K) {
  if (!expect(K))
    return false;
  consume();
  return true;
}

Token Parser::expectAndGet(TokenKind K, const char *Msg) {
  if (curTok().isNot(K)) {
    Diags.error(curTok().Loc, Msg);
    return Token(TokenKind::Unknown, curTok().Loc);
  }
  return consume();
}

bool Parser::isTypeKeyword(TokenKind K) {
  switch (K) {
  case TokenKind::KwVoid:
  case TokenKind::KwBool:
  case TokenKind::KwI8:
  case TokenKind::KwI16:
  case TokenKind::KwI32:
  case TokenKind::KwI64:
  case TokenKind::KwU8:
  case TokenKind::KwU16:
  case TokenKind::KwU32:
  case TokenKind::KwU64:
  case TokenKind::KwF32:
  case TokenKind::KwF64:
    return true;
  default:
    return false;
  }
}

//===----------------------------------------------------------------------===//
// Type parsing
//===----------------------------------------------------------------------===//

std::unique_ptr<TypeNode> Parser::parseType() {
  auto Loc = curTok().Loc;

  if (curTok().is(TokenKind::Identifier)) {
    auto Name = consume().getText();
    if (curTok().is(TokenKind::Star)) {
      consume();
      bool IsMut = false;
      if (curTok().is(TokenKind::KwMut)) {
        consume();
        IsMut = true;
      }
      return std::make_unique<TypeNode>(
          TypeNode::Pointer{
              std::make_unique<TypeNode>(TypeNode::Named{std::string(Name)},
                                         Loc),
              IsMut},
          Loc);
    }
    return std::make_unique<TypeNode>(TypeNode::Named{std::string(Name)}, Loc);
  }

  auto getBuiltin = [&](BuiltinTypeKind K) {
    consume();
    return std::make_unique<TypeNode>(TypeNode::Builtin{K}, Loc);
  };

  switch (curTok().Kind) {
  case TokenKind::KwVoid:
    return getBuiltin(BuiltinTypeKind::Void);
  case TokenKind::KwBool:
    return getBuiltin(BuiltinTypeKind::Bool);
  case TokenKind::KwI8:
    return getBuiltin(BuiltinTypeKind::I8);
  case TokenKind::KwI16:
    return getBuiltin(BuiltinTypeKind::I16);
  case TokenKind::KwI32:
    return getBuiltin(BuiltinTypeKind::I32);
  case TokenKind::KwI64:
    return getBuiltin(BuiltinTypeKind::I64);
  case TokenKind::KwU8:
    return getBuiltin(BuiltinTypeKind::U8);
  case TokenKind::KwU16:
    return getBuiltin(BuiltinTypeKind::U16);
  case TokenKind::KwU32:
    return getBuiltin(BuiltinTypeKind::U32);
  case TokenKind::KwU64:
    return getBuiltin(BuiltinTypeKind::U64);
  case TokenKind::KwF32:
    return getBuiltin(BuiltinTypeKind::F32);
  case TokenKind::KwF64:
    return getBuiltin(BuiltinTypeKind::F64);
  default:
    return nullptr;
  }
}

//===----------------------------------------------------------------------===//
// Expression parsing (precedence climbing)
//===----------------------------------------------------------------------===//

std::unique_ptr<Expr> Parser::parseExpr() {
  return parseAssignment();
}

std::unique_ptr<Expr> Parser::parseAssignment() {
  auto LHS = parseTernary();
  if (curTok().is(TokenKind::Eq)) {
    auto Loc = curTok().Loc;
    consume();
    auto Val = parseAssignment();
    if (auto *Id = dynamic_cast<IdentifierExpr *>(LHS.get())) {
      return std::make_unique<AssignmentExpr>(Id->Name, std::move(Val), Loc);
    }
    Diags.error(Loc, "invalid assignment target");
    return Val;
  }
  return LHS;
}

std::unique_ptr<Expr> Parser::parseTernary() {
  return parseLogicalOr();
}

std::unique_ptr<Expr> Parser::parseLogicalOr() {
  auto LHS = parseLogicalAnd();
  while (curTok().is(TokenKind::PipePipe)) {
    auto Loc = curTok().Loc;
    consume();
    auto RHS = parseLogicalAnd();
    LHS = std::make_unique<BinaryExpr>(BinaryOp::Or, std::move(LHS),
                                       std::move(RHS), Loc);
  }
  return LHS;
}

std::unique_ptr<Expr> Parser::parseLogicalAnd() {
  auto LHS = parseEquality();
  while (curTok().is(TokenKind::AmpAmp)) {
    auto Loc = curTok().Loc;
    consume();
    auto RHS = parseEquality();
    LHS = std::make_unique<BinaryExpr>(BinaryOp::And, std::move(LHS),
                                        std::move(RHS), Loc);
  }
  return LHS;
}

std::unique_ptr<Expr> Parser::parseEquality() {
  auto LHS = parseRelational();
  while (curTok().isOneOf(TokenKind::EqEq, TokenKind::ExclaimEq)) {
    auto Loc = curTok().Loc;
    auto Op = curTok().is(TokenKind::EqEq) ? BinaryOp::Eq : BinaryOp::Ne;
    consume();
    auto RHS = parseRelational();
    LHS = std::make_unique<BinaryExpr>(Op, std::move(LHS), std::move(RHS), Loc);
  }
  return LHS;
}

std::unique_ptr<Expr> Parser::parseRelational() {
  auto LHS = parseShift();
  while (curTok().isOneOf(TokenKind::Less, TokenKind::Greater,
                          TokenKind::LessEq, TokenKind::GreaterEq)) {
    auto Loc = curTok().Loc;
    BinaryOp Op;
    switch (curTok().Kind) {
    case TokenKind::Less: Op = BinaryOp::Lt; break;
    case TokenKind::Greater: Op = BinaryOp::Gt; break;
    case TokenKind::LessEq: Op = BinaryOp::Le; break;
    case TokenKind::GreaterEq: Op = BinaryOp::Ge; break;
    default: Op = BinaryOp::Eq;
    }
    consume();
    auto RHS = parseShift();
    LHS = std::make_unique<BinaryExpr>(Op, std::move(LHS), std::move(RHS), Loc);
  }
  return LHS;
}

std::unique_ptr<Expr> Parser::parseShift() {
  auto LHS = parseAdditive();
  while (curTok().isOneOf(TokenKind::LessLess, TokenKind::GreaterGreater)) {
    auto Loc = curTok().Loc;
    auto Op = curTok().is(TokenKind::LessLess) ? BinaryOp::Shl : BinaryOp::Shr;
    consume();
    auto RHS = parseAdditive();
    LHS = std::make_unique<BinaryExpr>(Op, std::move(LHS), std::move(RHS), Loc);
  }
  return LHS;
}

std::unique_ptr<Expr> Parser::parseAdditive() {
  auto LHS = parseMultiplicative();
  while (curTok().isOneOf(TokenKind::Plus, TokenKind::Minus)) {
    auto Loc = curTok().Loc;
    auto Op = curTok().is(TokenKind::Plus) ? BinaryOp::Add : BinaryOp::Sub;
    consume();
    auto RHS = parseMultiplicative();
    LHS = std::make_unique<BinaryExpr>(Op, std::move(LHS), std::move(RHS), Loc);
  }
  return LHS;
}

std::unique_ptr<Expr> Parser::parseMultiplicative() {
  auto LHS = parseUnary();
  while (curTok().isOneOf(TokenKind::Star, TokenKind::Slash,
                          TokenKind::Percent)) {
    auto Loc = curTok().Loc;
    BinaryOp Op;
    switch (curTok().Kind) {
    case TokenKind::Star: Op = BinaryOp::Mul; break;
    case TokenKind::Slash: Op = BinaryOp::Div; break;
    case TokenKind::Percent: Op = BinaryOp::Mod; break;
    default: Op = BinaryOp::Mul;
    }
    consume();
    auto RHS = parseUnary();
    LHS = std::make_unique<BinaryExpr>(Op, std::move(LHS), std::move(RHS), Loc);
  }
  return LHS;
}

std::unique_ptr<Expr> Parser::parseUnary() {
  auto Loc = curTok().Loc;

  if (curTok().is(TokenKind::Minus)) {
    consume();
    auto Operand = parseUnary();
    return std::make_unique<UnaryExpr>(UnaryOp::Neg, std::move(Operand), Loc);
  }
  if (curTok().is(TokenKind::Exclaim)) {
    consume();
    auto Operand = parseUnary();
    return std::make_unique<UnaryExpr>(UnaryOp::Not, std::move(Operand), Loc);
  }
  if (curTok().is(TokenKind::Tilde)) {
    consume();
    auto Operand = parseUnary();
    return std::make_unique<UnaryExpr>(UnaryOp::BitNot, std::move(Operand), Loc);
  }

  return parsePostfix();
}

std::unique_ptr<Expr> Parser::parsePostfix() {
  auto LHS = parsePrimary();

  while (true) {
    if (curTok().is(TokenKind::LParen)) {
      auto Loc = curTok().Loc;
      consume();
      std::vector<std::unique_ptr<Expr>> Args;
      if (curTok().isNot(TokenKind::RParen)) {
        Args.push_back(parseExpr());
        while (curTok().is(TokenKind::Comma)) {
          consume();
          Args.push_back(parseExpr());
        }
      }
      expectAndConsume(TokenKind::RParen);
      if (auto *Id = dynamic_cast<IdentifierExpr *>(LHS.get())) {
        LHS = std::make_unique<CallExpr>(Id->Name, std::move(Args), Loc);
      } else {
        Diags.error(Loc, "cannot call non-identifier expression");
      }
    } else if (curTok().is(TokenKind::Dot)) {
      consume();
      auto Loc = curTok().Loc;
      auto Member = expectAndGet(TokenKind::Identifier, "expected member name");
      LHS = std::make_unique<MemberAccessExpr>(std::move(LHS),
                                                std::string(Member.getText()),
                                                Loc);
    } else if (curTok().is(TokenKind::LBracket)) {
      consume();
      auto Loc = curTok().Loc;
      auto Index = parseExpr();
      expectAndConsume(TokenKind::RBracket);
      LHS = std::make_unique<ArrayIndexExpr>(std::move(LHS), std::move(Index),
                                              Loc);
    } else {
      break;
    }
  }

  return LHS;
}

std::unique_ptr<Expr> Parser::parsePrimary() {
  auto Loc = curTok().Loc;

  if (curTok().is(TokenKind::IntegerLiteral)) {
    auto T = consume();
    int64_t Val = std::stoll(std::string(T.getText()));
    return std::make_unique<IntegerLiteralExpr>(Val, Loc);
  }
  if (curTok().is(TokenKind::FloatLiteral)) {
    auto T = consume();
    double Val = std::stod(std::string(T.getText()));
    return std::make_unique<FloatLiteralExpr>(Val, Loc);
  }
  if (curTok().is(TokenKind::StringLiteral)) {
    auto T = consume();
    return std::make_unique<StringLiteralExpr>(std::string(T.getText()), Loc);
  }
  if (curTok().is(TokenKind::CharLiteral)) {
    auto T = consume();
    char Val = T.getText().empty() ? '\0' : T.getText()[0];
    return std::make_unique<CharLiteralExpr>(Val, Loc);
  }
  if (curTok().is(TokenKind::KwTrue)) {
    consume();
    return std::make_unique<BoolLiteralExpr>(true, Loc);
  }
  if (curTok().is(TokenKind::KwFalse)) {
    consume();
    return std::make_unique<BoolLiteralExpr>(false, Loc);
  }
  if (curTok().is(TokenKind::Identifier)) {
    auto T = consume();
    return std::make_unique<IdentifierExpr>(std::string(T.getText()), Loc);
  }
  if (curTok().is(TokenKind::LParen)) {
    consume();
    auto E = parseExpr();
    expectAndConsume(TokenKind::RParen);
    return E;
  }

  Diags.error(Loc, "expected expression");
  return std::make_unique<IntegerLiteralExpr>(0, Loc);
}

//===----------------------------------------------------------------------===//
// Statement parsing
//===----------------------------------------------------------------------===//

std::unique_ptr<Stmt> Parser::parseStmt() {
  skipNewlines(Lex);

  if (curTok().is(TokenKind::Eof))
    return nullptr;

  switch (curTok().Kind) {
  case TokenKind::LBrace:
    return parseBlock();
  case TokenKind::KwIf:
    return parseIf();
  case TokenKind::KwWhile:
    return parseWhile();
  case TokenKind::KwFor:
    return parseFor();
  case TokenKind::KwReturn:
    return parseReturn();
  case TokenKind::KwBreak:
    return parseBreak();
  case TokenKind::KwContinue:
    return parseContinue();
  case TokenKind::KwSleep:
  case TokenKind::KwWait:
    return parseSleep();
  case TokenKind::KwSent:
    return parseSent();
  default:
    return parseExprStmt();
  }
}

//===----------------------------------------------------------------------===//
// Sleep / Wait statement
//===----------------------------------------------------------------------===//

std::unique_ptr<Stmt> Parser::parseSleep() {
  auto Loc = curTok().Loc;
  bool IsWait = curTok().is(TokenKind::KwWait);
  consume(); // sleep or wait

  auto Duration = parseExpr();

  // Parse time unit
  TokenKind Unit;
  switch (curTok().Kind) {
  case TokenKind::KwMs:    Unit = TokenKind::KwMs; break;
  case TokenKind::KwSec:   Unit = TokenKind::KwSec; break;
  case TokenKind::KwMin:   Unit = TokenKind::KwMin; break;
  case TokenKind::KwHour:  Unit = TokenKind::KwHour; break;
  case TokenKind::KwDay:   Unit = TokenKind::KwDay; break;
  case TokenKind::KwMonth: Unit = TokenKind::KwMonth; break;
  case TokenKind::KwYear:  Unit = TokenKind::KwYear; break;
  default:
    Diags.error(curTok().Loc, "expected time unit (ms, s, min, h, d, m, y)");
    Unit = TokenKind::KwSec;
    break;
  }
  consume();

  stmtSep(Lex);
  return std::make_unique<SleepStmt>(std::move(Duration), Unit, IsWait, Loc);
}

//===----------------------------------------------------------------------===//
// Sent (network request) statement
//===----------------------------------------------------------------------===//

static SentProtocol parseSentProtocol(TokenKind K) {
  switch (K) {
  case TokenKind::KwHttp:  return SentProtocol::HTTP;
  case TokenKind::KwHttps: return SentProtocol::HTTPS;
  case TokenKind::KwTcp:   return SentProtocol::TCP;
  case TokenKind::KwUdp:   return SentProtocol::UDP;
  default:                 return SentProtocol::HTTP;
  }
}

static SentMode parseSentMode(TokenKind K) {
  return K == TokenKind::KwManual ? SentMode::Manual : SentMode::Auto;
}

static SentManualType parseSentManualType(TokenKind K) {
  switch (K) {
  case TokenKind::KwHex:    return SentManualType::Hex;
  case TokenKind::KwBinary: return SentManualType::Binary;
  default:                  return SentManualType::Text;
  }
}

std::unique_ptr<Stmt> Parser::parseSent() {
  auto Loc = curTok().Loc;
  consume(); // sent

  // Protocol
  if (!curTok().isOneOf(TokenKind::KwHttp, TokenKind::KwHttps,
                        TokenKind::KwTcp, TokenKind::KwUdp)) {
    Diags.error(curTok().Loc, "expected protocol (http, https, tcp, udp)");
    return std::make_unique<ExprStmt>(
        std::make_unique<IntegerLiteralExpr>(0, Loc), Loc);
  }
  SentProtocol Proto = parseSentProtocol(curTok().Kind);
  consume();

  // Mode
  if (!curTok().isOneOf(TokenKind::KwAuto, TokenKind::KwManual)) {
    Diags.error(curTok().Loc, "expected mode (auto, manual)");
    return std::make_unique<ExprStmt>(
        std::make_unique<IntegerLiteralExpr>(0, Loc), Loc);
  }
  SentMode SendMode = parseSentMode(curTok().Kind);
  consume();

  std::string RequestContent;
  SentManualType ManType = SentManualType::Text;
  std::string ManualData;
  std::string Address;
  uint16_t Port = 0;
  std::vector<std::pair<std::string, std::string>> Headers;
  bool HasHeaders = false;
  std::unique_ptr<Expr> Payload;
  bool HasPayload = false;
  bool IsRaw = false;

  if (SendMode == SentMode::Auto) {
    // Request content string
    if (!curTok().is(TokenKind::StringLiteral)) {
      Diags.error(curTok().Loc, "expected request content string");
      return std::make_unique<ExprStmt>(
          std::make_unique<IntegerLiteralExpr>(0, Loc), Loc);
    }
    auto T = consume();
    RequestContent = std::string(T.getText());

    // Address [host:port]
    if (!curTok().is(TokenKind::LBracket)) {
      Diags.error(curTok().Loc, "expected target address [host:port]");
      return std::make_unique<ExprStmt>(
          std::make_unique<IntegerLiteralExpr>(0, Loc), Loc);
    }
    consume(); // [
    Address.clear();
    while (curTok().is(TokenKind::Identifier) || curTok().is(TokenKind::Dot)) {
      if (curTok().is(TokenKind::Dot)) {
        Address += ".";
        consume();
      } else {
        Address += std::string(consume().getText());
      }
    }
    // Check for :port
    if (curTok().is(TokenKind::Colon)) {
      consume();
      auto PortStr = expectAndGet(TokenKind::IntegerLiteral, "expected port");
      Port = static_cast<uint16_t>(std::stoi(std::string(PortStr.getText())));
    }
    expectAndConsume(TokenKind::RBracket);
    if (Address.empty()) {
      Diags.error(Loc, "empty address");
    }

    // Optional headers block
    if (curTok().is(TokenKind::KwHeaders)) {
      consume();
      HasHeaders = true;
      expectAndConsume(TokenKind::LBrace);
      while (curTok().isNot(TokenKind::RBrace) &&
             curTok().isNot(TokenKind::Eof)) {
        skipNewlines(Lex);
        if (curTok().is(TokenKind::RBrace)) break;
        auto KeyTok = expectAndGet(TokenKind::StringLiteral, "expected header key");
        expectAndConsume(TokenKind::Colon);
        auto ValTok = expectAndGet(TokenKind::StringLiteral, "expected header value");
        Headers.push_back({std::string(KeyTok.getText()),
                           std::string(ValTok.getText())});
        skipNewlines(Lex);
      }
      expectAndConsume(TokenKind::RBrace);
    }

    // Optional payload
    if (curTok().is(TokenKind::KwPayload)) {
      consume();
      HasPayload = true;
      Payload = parseExpr();
    }

    // Optional raw flag
    if (curTok().is(TokenKind::KwRaw)) {
      consume();
      IsRaw = true;
    }
  } else {
    // Manual mode: data type + data + address
    if (!curTok().isOneOf(TokenKind::KwHex, TokenKind::KwBinary,
                          TokenKind::KwText)) {
      Diags.error(curTok().Loc, "expected data type (hex, binary, text)");
      return std::make_unique<ExprStmt>(
          std::make_unique<IntegerLiteralExpr>(0, Loc), Loc);
    }
    ManType = parseSentManualType(curTok().Kind);
    consume();

    auto DataTok = expectAndGet(TokenKind::StringLiteral, "expected data string");
    ManualData = std::string(DataTok.getText());

    // Address [host:port]
    if (!curTok().is(TokenKind::LBracket)) {
      Diags.error(curTok().Loc, "expected target address [host:port]");
      return std::make_unique<ExprStmt>(
          std::make_unique<IntegerLiteralExpr>(0, Loc), Loc);
    }
    consume(); // [
    Address.clear();
    while (curTok().is(TokenKind::Identifier) || curTok().is(TokenKind::Dot)) {
      if (curTok().is(TokenKind::Dot)) {
        Address += ".";
        consume();
      } else {
        Address += std::string(consume().getText());
      }
    }
    if (curTok().is(TokenKind::Colon)) {
      consume();
      auto PortStr = expectAndGet(TokenKind::IntegerLiteral, "expected port");
      Port = static_cast<uint16_t>(std::stoi(std::string(PortStr.getText())));
    }
    expectAndConsume(TokenKind::RBracket);
    if (Address.empty()) {
      Diags.error(Loc, "empty address");
    }
  }

  stmtSep(Lex);
  return std::make_unique<SentStmt>(Proto, SendMode, RequestContent,
                                     ManType, ManualData,
                                     Address, Port,
                                     std::move(Headers), HasHeaders,
                                     std::move(Payload), HasPayload,
                                     IsRaw, Loc);
}

//===----------------------------------------------------------------------===//
// Other statements
//===----------------------------------------------------------------------===//

std::unique_ptr<Stmt> Parser::parseBlock() {
  auto Loc = curTok().Loc;
  consume(); // {

  std::vector<std::unique_ptr<Stmt>> Stmts;
  while (curTok().isNot(TokenKind::RBrace) && curTok().isNot(TokenKind::Eof)) {
    auto S = parseStmt();
    if (S)
      Stmts.push_back(std::move(S));
  }

  expectAndConsume(TokenKind::RBrace);
  return std::make_unique<BlockStmt>(std::move(Stmts), Loc);
}

std::unique_ptr<Stmt> Parser::parseIf() {
  auto Loc = curTok().Loc;
  consume(); // if

  auto Cond = parseExpr();
  auto Then = parseStmt();

  std::unique_ptr<Stmt> Else;
  skipNewlines(Lex);
  if (curTok().is(TokenKind::KwElse)) {
    consume();
    Else = parseStmt();
  }

  return std::make_unique<IfStmt>(std::move(Cond), std::move(Then),
                                  std::move(Else), Loc);
}

std::unique_ptr<Stmt> Parser::parseWhile() {
  auto Loc = curTok().Loc;
  consume(); // while

  auto Cond = parseExpr();
  auto Body = parseStmt();

  return std::make_unique<WhileStmt>(std::move(Cond), std::move(Body), Loc);
}

std::unique_ptr<Stmt> Parser::parseFor() {
  auto Loc = curTok().Loc;
  consume(); // for

  auto VarTok = expectAndGet(TokenKind::Identifier, "expected loop variable name");
  std::string VarName = std::string(VarTok.getText());

  expectAndConsume(TokenKind::KwIn);

  std::unique_ptr<Expr> Start;
  std::unique_ptr<Expr> End;

  Start = parseExpr();

  if (curTok().is(TokenKind::DotDot)) {
    consume();
    End = parseExpr();
  } else {
    End = std::move(Start);
    Start = std::make_unique<IntegerLiteralExpr>(0, Loc);
  }

  auto Body = parseStmt();
  return std::make_unique<ForStmt>(VarName, std::move(Start), std::move(End),
                                   std::move(Body), Loc);
}

std::unique_ptr<Stmt> Parser::parseReturn() {
  auto Loc = curTok().Loc;
  consume(); // return

  std::unique_ptr<Expr> Val;
  skipNewlines(Lex);
  if (curTok().isNot(TokenKind::RBrace) && curTok().isNot(TokenKind::Eof) &&
      curTok().isNot(TokenKind::Semi)) {
    Val = parseExpr();
  }

  stmtSep(Lex);
  return std::make_unique<ReturnStmt>(std::move(Val), Loc);
}

std::unique_ptr<Stmt> Parser::parseBreak() {
  auto Loc = curTok().Loc;
  consume();
  stmtSep(Lex);
  return std::make_unique<BreakStmt>(Loc);
}

std::unique_ptr<Stmt> Parser::parseContinue() {
  auto Loc = curTok().Loc;
  consume();
  stmtSep(Lex);
  return std::make_unique<ContinueStmt>(Loc);
}

std::unique_ptr<Stmt> Parser::parseExprStmt() {
  auto Loc = curTok().Loc;
  auto E = parseExpr();
  stmtSep(Lex);
  return std::make_unique<ExprStmt>(std::move(E), Loc);
}

//===----------------------------------------------------------------------===//
// Function and top-level declarations
//===----------------------------------------------------------------------===//

ParamDecl Parser::parseParam() {
  auto Loc = curTok().Loc;
  auto NameTok = expectAndGet(TokenKind::Identifier, "expected parameter name");
  std::string Name = std::string(NameTok.getText());

  std::unique_ptr<TypeNode> ParamType;
  if (curTok().is(TokenKind::Colon)) {
    consume();
    ParamType = parseType();
  }

  return ParamDecl(Name, std::move(ParamType), Loc);
}

FunctionDecl Parser::parseFunction(bool IsPub) {
  auto Loc = curTok().Loc;

  // Optional return type before name (C++ style compat, removed in new syntax)
  // New syntax: func name(params) { body }

  // If we just consumed `func`, the next token is the name
  auto NameTok = expectAndGet(TokenKind::Identifier, "expected function name");
  std::string Name = std::string(NameTok.getText());

  expectAndConsume(TokenKind::LParen);

  std::vector<ParamDecl> Params;
  if (curTok().isNot(TokenKind::RParen)) {
    Params.push_back(parseParam());
    while (curTok().is(TokenKind::Comma)) {
      consume();
      Params.push_back(parseParam());
    }
  }
  expectAndConsume(TokenKind::RParen);

  // Optional return type with arrow syntax: -> type
  std::unique_ptr<TypeNode> ReturnType;
  if (curTok().is(TokenKind::Arrow)) {
    consume();
    ReturnType = parseType();
  }

  auto Body = parseBlock();
  return FunctionDecl(IsPub, Name, std::move(Params), std::move(ReturnType),
                      std::unique_ptr<BlockStmt>(
                          static_cast<BlockStmt *>(Body.release())),
                      Loc);
}

StructDecl Parser::parseStruct(bool IsPub) {
  auto Loc = curTok().Loc;
  consume(); // struct

  auto NameTok = expectAndGet(TokenKind::Identifier, "expected struct name");
  std::string Name = std::string(NameTok.getText());

  expectAndConsume(TokenKind::LBrace);

  std::vector<StructMember> Members;
  while (curTok().isNot(TokenKind::RBrace) && curTok().isNot(TokenKind::Eof)) {
    skipNewlines(Lex);
    if (curTok().is(TokenKind::RBrace))
      break;

    bool IsMemberPub = false;
    if (curTok().is(TokenKind::KwPub)) {
      IsMemberPub = true;
      consume();
    }

    auto MemberTok = expectAndGet(TokenKind::Identifier, "expected member name");
    std::string MemberName = std::string(MemberTok.getText());

    std::unique_ptr<TypeNode> MemberType;
    if (curTok().is(TokenKind::Colon)) {
      consume();
      MemberType = parseType();
    }

    Members.push_back({MemberName, std::move(MemberType), IsMemberPub});
    stmtSep(Lex);
  }

  expectAndConsume(TokenKind::RBrace);
  return StructDecl{Name, std::move(Members), IsPub, Loc};
}

//===----------------------------------------------------------------------===//
// Top-level declarations
//===----------------------------------------------------------------------===//

std::unique_ptr<OtherhaveDecl> Parser::parseOtherhave() {
  auto Loc = curTok().Loc;
  consume(); // otherhave
  expectAndConsume(TokenKind::LParen);
  std::vector<std::string> Languages;
  if (curTok().isNot(TokenKind::RParen)) {
    auto T = expectAndGet(TokenKind::Identifier, "expected language name");
    Languages.push_back(std::string(T.getText()));
    while (curTok().is(TokenKind::Comma)) {
      consume();
      auto T2 = expectAndGet(TokenKind::Identifier, "expected language name");
      Languages.push_back(std::string(T2.getText()));
    }
  }
  expectAndConsume(TokenKind::RParen);
  stmtSep(Lex);
  return std::make_unique<OtherhaveDecl>(Languages, Loc);
}

std::unique_ptr<AnnotateDecl> Parser::parseAnnotate() {
  auto Loc = curTok().Loc;
  consume(); // annotate
  expectAndConsume(TokenKind::LBrace);
  std::string Symbol1, Symbol2, Type;
  while (curTok().isNot(TokenKind::RBrace) && curTok().isNot(TokenKind::Eof)) {
    skipNewlines(Lex);
    if (curTok().is(TokenKind::RBrace)) break;
    auto Key = expectAndGet(TokenKind::Identifier, "expected key");
    std::string KeyStr = std::string(Key.getText());
    expectAndConsume(TokenKind::Colon);
    auto Val = expectAndGet(TokenKind::StringLiteral, "expected value string");
    std::string ValStr = std::string(Val.getText());
    if (KeyStr == "symbol1") Symbol1 = ValStr;
    else if (KeyStr == "symbol2") Symbol2 = ValStr;
    else if (KeyStr == "type") Type = ValStr;
    skipNewlines(Lex);
  }
  expectAndConsume(TokenKind::RBrace);
  return std::make_unique<AnnotateDecl>(Symbol1, Symbol2, Type, Loc);
}

std::unique_ptr<PackageDecl> Parser::parsePackage() {
  auto Loc = curTok().Loc;
  consume(); // package
  expectAndConsume(TokenKind::LParen);
  auto NameTok = expectAndGet(TokenKind::StringLiteral, "expected package name string");
  std::string Name = std::string(NameTok.getText());
  expectAndConsume(TokenKind::RParen);
  stmtSep(Lex);
  return std::make_unique<PackageDecl>(Name, Loc);
}

//===----------------------------------------------------------------------===//
// Program
//===----------------------------------------------------------------------===//

std::unique_ptr<Program> Parser::parseProgram() {
  auto Prog = std::make_unique<Program>();

  while (curTok().isNot(TokenKind::Eof)) {
    skipNewlines(Lex);
    if (curTok().is(TokenKind::Eof))
      break;

    // File structure (fixed order):
    // 1. otherhave (optional)
    // 2. annotate (optional)
    // 3. package (optional)
    // 4. in / import (optional, multiple)
    // 5. func / pub / struct / import

    if (curTok().is(TokenKind::KwOtherhave)) {
      if (Prog->Otherhave)
        Diags.error(curTok().Loc, "duplicate otherhave declaration");
      else
        Prog->Otherhave = parseOtherhave();
    } else if (curTok().is(TokenKind::KwAnnotate)) {
      if (Prog->Annotate)
        Diags.error(curTok().Loc, "duplicate annotate block");
      else
        Prog->Annotate = parseAnnotate();
    } else if (curTok().is(TokenKind::KwPackage)) {
      if (Prog->Package)
        Diags.error(curTok().Loc, "duplicate package declaration");
      else
        Prog->Package = parsePackage();
    } else if (curTok().is(TokenKind::KwIn) || curTok().is(TokenKind::KwImport)) {
      consume();
      auto ModTok = expectAndGet(TokenKind::StringLiteral, "expected module name string");
      Prog->Imports.push_back({std::string(ModTok.getText()), ModTok.Loc});
      stmtSep(Lex);
    } else if (curTok().isOneOf(TokenKind::KwFunc, TokenKind::KwFn)) {
      // `func` is the new keyword, `fn` is deprecated compat
      if (curTok().is(TokenKind::KwFn))
        consume(); // skip deprecated fn
      // consume func if present (kwFunc case doesn't need consume)
      // Actually for KwFunc, we need to consume it before parseFunction
      // But parseFunction expects the name as the first token
      consume(); // consume func/fn keyword
      Prog->Functions.push_back(parseFunction());
    } else if (curTok().is(TokenKind::KwPub)) {
      consume();
      if (curTok().is(TokenKind::KwStruct)) {
        Prog->Structs.push_back(parseStruct(true));
      } else if (curTok().isOneOf(TokenKind::KwFunc, TokenKind::KwFn)) {
        if (curTok().is(TokenKind::KwFn)) consume(); // deprecated fn
        consume(); // func
        Prog->Functions.push_back(parseFunction(true));
      } else {
        Diags.error(curTok().Loc, "expected func or struct after pub");
      }
    } else if (curTok().is(TokenKind::KwStruct)) {
      Prog->Structs.push_back(parseStruct());
    } else {
      Diags.error(curTok().Loc,
                  "expected otherhave, annotate, package, in, func, pub, or struct");
      consume();
    }
  }

  return Prog;
}
