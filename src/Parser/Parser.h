#ifndef CITIUS_PARSER_H
#define CITIUS_PARSER_H

#include "Lexer/Lexer.h"
#include "AST/AST.h"
#include "Basic/Diagnostic.h"
#include <memory>

class Parser {
public:
  Parser(DiagnosticsEngine &Diags, Lexer &Lex)
      : Diags(Diags), Lex(Lex) {}

  std::unique_ptr<Program> parseProgram();

private:
  DiagnosticsEngine &Diags;
  Lexer &Lex;

  Token curTok() { return Lex.peekToken(); }
  Token consume() { return Lex.nextToken(); }
  Token advance() {
    auto T = Lex.peekToken();
    Lex.nextToken();
    return T;
  }

  bool expect(TokenKind K);
  bool expectAndConsume(TokenKind K);
  Token expectAndGet(TokenKind K, const char *Msg);
  bool isTypeKeyword(TokenKind K);

  std::unique_ptr<TypeNode> parseType();
  std::unique_ptr<Expr> parseExpr();
  std::unique_ptr<Expr> parseAssignment();
  std::unique_ptr<Expr> parseTernary();
  std::unique_ptr<Expr> parseLogicalOr();
  std::unique_ptr<Expr> parseLogicalAnd();
  std::unique_ptr<Expr> parseEquality();
  std::unique_ptr<Expr> parseRelational();
  std::unique_ptr<Expr> parseShift();
  std::unique_ptr<Expr> parseAdditive();
  std::unique_ptr<Expr> parseMultiplicative();
  std::unique_ptr<Expr> parseUnary();
  std::unique_ptr<Expr> parsePostfix();
  std::unique_ptr<Expr> parsePrimary();
  std::unique_ptr<Expr> parseCallOrIdentifier(IdentifierExpr *Id);

  std::unique_ptr<Stmt> parseStmt();
  std::unique_ptr<Stmt> parseVarDecl();
  std::unique_ptr<Stmt> parseBlock();
  std::unique_ptr<Stmt> parseIf();
  std::unique_ptr<Stmt> parseWhile();
  std::unique_ptr<Stmt> parseFor();
  std::unique_ptr<Stmt> parseReturn();
  std::unique_ptr<Stmt> parseBreak();
  std::unique_ptr<Stmt> parseContinue();
  std::unique_ptr<Stmt> parseExprStmt();
  std::unique_ptr<Stmt> parseSleep();
  std::unique_ptr<Stmt> parseSent();

  std::unique_ptr<OtherhaveDecl> parseOtherhave();
  std::unique_ptr<AnnotateDecl> parseAnnotate();
  std::unique_ptr<PackageDecl> parsePackage();

  ParamDecl parseParam();
  FunctionDecl parseFunction(bool IsPub = false);
  StructDecl parseStruct(bool IsPub = false);

  BinaryOp tokenToBinaryOp(TokenKind K);
};

#endif
