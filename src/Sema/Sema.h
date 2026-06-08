#ifndef CITIUS_SEMA_H
#define CITIUS_SEMA_H

#include "AST/AST.h"
#include "Basic/Diagnostic.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct VarInfo {
  std::string Name;
  std::unique_ptr<TypeNode> Type;
  bool IsMutable;
  bool IsInitialized;
};

struct FuncInfo {
  std::string Name;
  std::vector<VarInfo> Params;
  std::unique_ptr<TypeNode> ReturnType;
  bool IsPub;
  bool IsDefined;
  FunctionDecl *Decl{nullptr}; // AST node for type propagation
};

struct Scope {
  std::unordered_map<std::string, VarInfo> Vars;
  Scope *Parent{nullptr};
  bool IsLoop{false};

  Scope(Scope *Parent = nullptr) : Parent(Parent) {}

  VarInfo *lookup(const std::string &Name) {
    auto It = Vars.find(Name);
    if (It != Vars.end())
      return &It->second;
    if (Parent)
      return Parent->lookup(Name);
    return nullptr;
  }

  bool addVar(VarInfo Info) {
    return Vars.insert({Info.Name, std::move(Info)}).second;
  }
};

class Sema {
public:
  Sema(DiagnosticsEngine &Diags) : Diags(Diags), CurrentScope(nullptr) {}

  bool analyze(Program &Prog);

private:
  DiagnosticsEngine &Diags;
  Scope *CurrentScope;
  std::vector<std::unique_ptr<Scope>> Scopes;
  std::unordered_map<std::string, FuncInfo> Functions;
  bool InferredAny{false};

  void enterScope();
  void exitScope();
  Scope *currentScope() { return CurrentScope; }

  bool analyzeFunction(FunctionDecl &Fn);
  bool analyzeStmt(Stmt &S);
  bool analyzeExpr(Expr &E, TypeNode *Expected = nullptr);
  bool analyzeVarDecl(VarDeclStmt &VD);

  TypeNode *getExprType(Expr &E) { return E.ResultType.get(); }
  void setExprType(Expr &E, std::unique_ptr<TypeNode> T) {
    E.ResultType = std::move(T);
  }

  bool typesMatch(TypeNode *A, TypeNode *B);
  bool isNumericType(TypeNode *T);
  bool isIntegerType(TypeNode *T);
  bool isFloatType(TypeNode *T);
  BuiltinTypeKind getTypeKind(TypeNode *T);
  bool isVoidType(TypeNode *T);
  bool isBoolType(TypeNode *T);

  std::unique_ptr<TypeNode>
  getCommonType(TypeNode *A, TypeNode *B);
  std::unique_ptr<TypeNode> getBuiltinType(BuiltinTypeKind K);
};

#endif
