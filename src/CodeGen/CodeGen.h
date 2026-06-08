#ifndef CITIUS_CODEGEN_H
#define CITIUS_CODEGEN_H

#include "AST/AST.h"
#include "Basic/Diagnostic.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <sstream>
#include <vector>

class CodeGen {
public:
  CodeGen(DiagnosticsEngine &Diags) : Diags(Diags), IndentLevel(0),
      NeedsWinsock(false), NeedsSleep(false) {}

  bool generate(Program &Prog, std::string &Output);
  bool compileToExecutable(const std::string &SourcePath,
                           const std::string &OutputPath,
                           int OptLevel = 2);
  bool compileToObject(const std::string &SourcePath,
                       const std::string &OutputPath, int OptLevel = 2);

private:
  DiagnosticsEngine &Diags;
  std::ostringstream Output;
  int IndentLevel;
  bool NeedsWinsock;
  bool NeedsSleep;

  struct VarInfo {
    std::string Name;
    std::unique_ptr<TypeNode> Type;
    bool IsMutable;
  };

  std::unordered_map<std::string, VarInfo> VarScope;

  std::string indent();
  std::string mangleType(TypeNode *T);
  std::string getCTypeName(TypeNode *T);
  std::string getBuiltinCType(BuiltinTypeKind K);
  std::string genExpr(Expr &E);
  std::string genBinaryOp(BinaryOp Op, const std::string &LHS,
                          const std::string &RHS, TypeNode *Ty);
  void genStmt(Stmt &S);
  void genVarDecl(VarDeclStmt &VD);
  void genFunction(FunctionDecl &Fn);

  void genSleepStmt(class SleepStmt &S);
  void genSentStmt(class SentStmt &S);
  void scanStmt(Stmt *S);

  void emitNetHelpers();
  void emitSleepHelpers();

  void emit(const std::string &Str);
  void emitLine(const std::string &Str = "");
  void emitInclude(const std::string &Header);
};

#endif
