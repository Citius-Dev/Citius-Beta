#include "Sema/Sema.h"
#include <cassert>

//===----------------------------------------------------------------------===//
// Type helpers
//===----------------------------------------------------------------------===//

BuiltinTypeKind Sema::getTypeKind(TypeNode *T) {
  if (!T)
    return BuiltinTypeKind::Void;
  if (auto *B = std::get_if<TypeNode::Builtin>(&T->Data))
    return B->Kind;
  return BuiltinTypeKind::I32; // default fallback
}

bool Sema::isVoidType(TypeNode *T) {
  return T && std::holds_alternative<TypeNode::Builtin>(T->Data) &&
         std::get<TypeNode::Builtin>(T->Data).Kind == BuiltinTypeKind::Void;
}

bool Sema::isBoolType(TypeNode *T) {
  return T && std::holds_alternative<TypeNode::Builtin>(T->Data) &&
         std::get<TypeNode::Builtin>(T->Data).Kind == BuiltinTypeKind::Bool;
}

bool Sema::isNumericType(TypeNode *T) {
  return isIntegerType(T) || isFloatType(T);
}

bool Sema::isIntegerType(TypeNode *T) {
  if (!T || !std::holds_alternative<TypeNode::Builtin>(T->Data))
    return false;
  auto K = std::get<TypeNode::Builtin>(T->Data).Kind;
  switch (K) {
  case BuiltinTypeKind::I8:
  case BuiltinTypeKind::I16:
  case BuiltinTypeKind::I32:
  case BuiltinTypeKind::I64:
  case BuiltinTypeKind::U8:
  case BuiltinTypeKind::U16:
  case BuiltinTypeKind::U32:
  case BuiltinTypeKind::U64:
    return true;
  default:
    return false;
  }
}

bool Sema::isFloatType(TypeNode *T) {
  if (!T || !std::holds_alternative<TypeNode::Builtin>(T->Data))
    return false;
  auto K = std::get<TypeNode::Builtin>(T->Data).Kind;
  return K == BuiltinTypeKind::F32 || K == BuiltinTypeKind::F64;
}

bool Sema::typesMatch(TypeNode *A, TypeNode *B) {
  if (!A || !B)
    return true;
  if (A->Data.index() != B->Data.index())
    return false;

  if (auto *BA = std::get_if<TypeNode::Builtin>(&A->Data)) {
    auto *BB = std::get_if<TypeNode::Builtin>(&B->Data);
    return BB && BA->Kind == BB->Kind;
  }
  if (auto *NA = std::get_if<TypeNode::Named>(&A->Data)) {
    auto *NB = std::get_if<TypeNode::Named>(&B->Data);
    return NB && NA->Name == NB->Name;
  }
  // Pointer comparison
  if (auto *PA = std::get_if<TypeNode::Pointer>(&A->Data)) {
    auto *PB = std::get_if<TypeNode::Pointer>(&B->Data);
    return PB && typesMatch(PA->Pointee.get(), PB->Pointee.get());
  }

  return false;
}

std::unique_ptr<TypeNode>
Sema::getCommonType(TypeNode *A, TypeNode *B) {
  if (!A)
    return B ? B->clone() : nullptr;
  if (!B)
    return A->clone();
  if (typesMatch(A, B))
    return A->clone();

  // Numeric promotion
  if (isNumericType(A) && isNumericType(B)) {
    if (isFloatType(A) || isFloatType(B))
      return getBuiltinType(BuiltinTypeKind::F64);
    return getBuiltinType(BuiltinTypeKind::I64);
  }

  return A->clone();
}

std::unique_ptr<TypeNode> Sema::getBuiltinType(BuiltinTypeKind K) {
  auto Loc = SourceLocation();
  return std::make_unique<TypeNode>(TypeNode::Builtin{K}, Loc);
}

//===----------------------------------------------------------------------===//
// Scope management
//===----------------------------------------------------------------------===//

void Sema::enterScope() {
  Scopes.push_back(std::make_unique<Scope>(CurrentScope));
  CurrentScope = Scopes.back().get();
}

void Sema::exitScope() {
  assert(CurrentScope && "no scope to exit");
  CurrentScope = CurrentScope->Parent;
}

//===----------------------------------------------------------------------===//
// Main analysis
//===----------------------------------------------------------------------===//

bool Sema::analyze(Program &Prog) {
  // Register built-in functions
  auto registerBuiltin = [&](const std::string &Name, std::unique_ptr<TypeNode> RetTy,
                             std::vector<VarInfo> Params) {
    FuncInfo Info;
    Info.Name = Name;
    Info.ReturnType = std::move(RetTy);
    Info.IsPub = true;
    Info.IsDefined = true;
    Info.Params = std::move(Params);
    Functions[Name] = std::move(Info);
  };

  registerBuiltin("print", getBuiltinType(BuiltinTypeKind::Void), {});
  registerBuiltin("printi", getBuiltinType(BuiltinTypeKind::Void), {});

  // First pass: register all functions
  for (auto &Fn : Prog.Functions) {
    FuncInfo Info;
    Info.Name = Fn.Name;
    if (!Fn.ReturnType)
      Fn.ReturnType = getBuiltinType(BuiltinTypeKind::I64);
    Info.ReturnType = Fn.ReturnType->clone();
    Info.IsPub = Fn.IsPub;
    Info.IsDefined = true;
    for (auto &P : Fn.Params) {
      VarInfo V;
      V.Name = P.Name;
      // Don't default to I64 — leave nullptr for inference later
      if (P.ParamType)
        V.Type = P.ParamType->clone();
      V.IsMutable = true;
      V.IsInitialized = true;
      Info.Params.push_back(std::move(V));
    }
    Info.Decl = &Fn;
    Functions[Fn.Name] = std::move(Info);
  }

  // Second pass: analyze function bodies
  for (auto &Fn : Prog.Functions) {
    if (!analyzeFunction(Fn))
      return false;
  }

  // Re-analyze functions if any parameter types were inferred from call sites
  if (InferredAny) {
    for (auto &Fn : Prog.Functions) {
      analyzeFunction(Fn);
    }
  }

  return !Diags.hasError();
}

bool Sema::analyzeFunction(FunctionDecl &Fn) {
  enterScope();

  // Add parameters to scope (types may be inferred later)
  for (auto &P : Fn.Params) {
    VarInfo V;
    V.Name = P.Name;
    V.Type = P.ParamType ? P.ParamType->clone() : nullptr;
    V.IsMutable = true;
    V.IsInitialized = true;
    CurrentScope->addVar(std::move(V));
  }

  // Analyze body
  if (Fn.Body) {
    for (auto &S : Fn.Body->Statements) {
      if (!analyzeStmt(*S))
        return false;
    }
  }

  exitScope();
  return true;
}

bool Sema::analyzeStmt(Stmt &S) {
  switch (S.StmtKind) {
  case Stmt::Kind::VarDecl:
    return analyzeVarDecl(static_cast<VarDeclStmt &>(S));
  case Stmt::Kind::ExprStmt:
    return analyzeExpr(*static_cast<ExprStmt &>(S).E);
  case Stmt::Kind::Block: {
    enterScope();
    auto &Block = static_cast<BlockStmt &>(S);
    for (auto &St : Block.Statements) {
      if (!analyzeStmt(*St))
        return false;
    }
    exitScope();
    return true;
  }
  case Stmt::Kind::If: {
    auto &If = static_cast<IfStmt &>(S);
    if (!analyzeExpr(*If.Condition))
      return false;
    if (!analyzeStmt(*If.ThenBranch))
      return false;
    if (If.ElseBranch && !analyzeStmt(*If.ElseBranch))
      return false;
    return true;
  }
  case Stmt::Kind::While: {
    auto &W = static_cast<WhileStmt &>(S);
    if (!analyzeExpr(*W.Condition))
      return false;
    enterScope();
    CurrentScope->IsLoop = true;
    if (!analyzeStmt(*W.Body))
      return false;
    exitScope();
    return true;
  }
  case Stmt::Kind::For: {
    auto &F = static_cast<ForStmt &>(S);
    enterScope();
    CurrentScope->IsLoop = true;
    if (F.Start && !analyzeExpr(*F.Start))
      return false;
    if (F.End && !analyzeExpr(*F.End))
      return false;
    VarInfo LoopVar;
    LoopVar.Name = F.VarName;
    LoopVar.Type = getBuiltinType(BuiltinTypeKind::I64);
    LoopVar.IsMutable = true;
    LoopVar.IsInitialized = true;
    CurrentScope->addVar(std::move(LoopVar));
    if (!analyzeStmt(*F.Body))
      return false;
    exitScope();
    return true;
  }
  case Stmt::Kind::Return: {
    auto &R = static_cast<ReturnStmt &>(S);
    if (R.Value) {
      if (!analyzeExpr(*R.Value))
        return false;
    }
    return true;
  }
  case Stmt::Kind::Break:
  case Stmt::Kind::Continue:
    return true;
  }
  return true;
}

bool Sema::analyzeVarDecl(VarDeclStmt &VD) {
  if (VD.Init) {
    if (!analyzeExpr(*VD.Init))
      return false;
  }

  VarInfo Info;
  Info.Name = VD.Name;
  if (VD.DeclType) {
    Info.Type = VD.DeclType->clone();
  } else if (VD.Init && VD.Init->ResultType) {
    Info.Type = VD.Init->ResultType->clone();
    VD.DeclType = VD.Init->ResultType->clone(); // propagate to AST for CodeGen
  } else {
    Info.Type = getBuiltinType(BuiltinTypeKind::I64);
  }
  Info.IsMutable = VD.IsMutable;
  Info.IsInitialized = VD.Init != nullptr;

  if (!CurrentScope->addVar(std::move(Info))) {
    Diags.error(VD.Loc, "redefinition of variable '" + VD.Name + "'");
    return false;
  }

  return true;
}

bool Sema::analyzeExpr(Expr &E, TypeNode *Expected) {
  switch (E.ExprKind) {
  case Expr::Kind::IntegerLit: {
    setExprType(E, getBuiltinType(BuiltinTypeKind::I64));
    return true;
  }
  case Expr::Kind::FloatLit: {
    setExprType(E, getBuiltinType(BuiltinTypeKind::F64));
    return true;
  }
  case Expr::Kind::StringLit: {
    setExprType(E, std::make_unique<TypeNode>(
                       TypeNode::Named{"str"}, E.Loc));
    return true;
  }
  case Expr::Kind::CharLit: {
    setExprType(E, getBuiltinType(BuiltinTypeKind::I8));
    return true;
  }
  case Expr::Kind::BoolLit: {
    setExprType(E, getBuiltinType(BuiltinTypeKind::Bool));
    return true;
  }
  case Expr::Kind::Identifier: {
    auto &Id = static_cast<IdentifierExpr &>(E);
    VarInfo *V = CurrentScope->lookup(Id.Name);
    if (!V) {
      // Check if it's a function name
      auto Fit = Functions.find(Id.Name);
      if (Fit == Functions.end()) {
        Diags.error(E.Loc, "use of undeclared identifier '" + Id.Name + "'");
        setExprType(E, getBuiltinType(BuiltinTypeKind::Void));
        return false;
      }
      // Function name as expression - give it void type
      setExprType(E, getBuiltinType(BuiltinTypeKind::Void));
      return true;
    }
    if (V->Type) {
      setExprType(E, V->Type->clone());
    }
    return true;
  }
  case Expr::Kind::Binary: {
    auto &Bin = static_cast<BinaryExpr &>(E);
    if (!analyzeExpr(*Bin.LHS) || !analyzeExpr(*Bin.RHS))
      return false;

    auto *LT = getExprType(*Bin.LHS);
    auto *RT = getExprType(*Bin.RHS);

    // Common type promotion
    auto Common = getCommonType(LT, RT);
    setExprType(E, Common ? Common->clone() : nullptr);

    // Check for valid binary ops (tolerate unresolved types)
    switch (Bin.Op) {
    case BinaryOp::Add:
    case BinaryOp::Sub:
    case BinaryOp::Mul:
    case BinaryOp::Div:
    case BinaryOp::Mod:
      if ((LT && !isNumericType(LT)) || (RT && !isNumericType(RT))) {
        Diags.error(Bin.Loc, "arithmetic operands must be numeric");
        return false;
      }
      break;
    case BinaryOp::Eq:
    case BinaryOp::Ne:
    case BinaryOp::Lt:
    case BinaryOp::Gt:
    case BinaryOp::Le:
    case BinaryOp::Ge:
      if ((LT && !isNumericType(LT)) || (RT && !isNumericType(RT))) {
        Diags.error(Bin.Loc, "comparison operands must be numeric");
        return false;
      }
      setExprType(E, getBuiltinType(BuiltinTypeKind::Bool));
      break;
    case BinaryOp::And:
    case BinaryOp::Or:
      if ((LT && !isBoolType(LT)) || (RT && !isBoolType(RT))) {
        Diags.error(Bin.Loc, "logical operands must be bool");
        return false;
      }
      setExprType(E, getBuiltinType(BuiltinTypeKind::Bool));
      break;
    case BinaryOp::BitAnd:
    case BinaryOp::BitOr:
    case BinaryOp::BitXor:
    case BinaryOp::Shl:
    case BinaryOp::Shr:
      if ((LT && !isIntegerType(LT)) || (RT && !isIntegerType(RT))) {
        Diags.error(Bin.Loc, "bitwise operands must be integers");
        return false;
      }
      break;
    }
    return true;
  }
  case Expr::Kind::Unary: {
    auto &Un = static_cast<UnaryExpr &>(E);
    if (!analyzeExpr(*Un.Operand))
      return false;
    setExprType(E, getExprType(*Un.Operand) ? getExprType(*Un.Operand)->clone()
                                            : nullptr);

    auto *OT = getExprType(*Un.Operand);
    switch (Un.Op) {
    case UnaryOp::Neg:
      if (OT && !isNumericType(OT)) {
        Diags.error(Un.Loc, "negation requires numeric operand");
        return false;
      }
      break;
    case UnaryOp::Not:
      if (OT && !isBoolType(OT)) {
        Diags.error(Un.Loc, "logical not requires bool operand");
        return false;
      }
      break;
    case UnaryOp::BitNot:
      if (OT && !isIntegerType(OT)) {
        Diags.error(Un.Loc, "bitwise not requires integer operand");
        return false;
      }
      break;
    }
    return true;
  }
  case Expr::Kind::Call: {
    auto &Call = static_cast<CallExpr &>(E);
    auto Fit = Functions.find(Call.Callee);
    if (Fit == Functions.end()) {
      Diags.error(E.Loc, "call to undeclared function '" + Call.Callee + "'");
      setExprType(E, getBuiltinType(BuiltinTypeKind::Void));
      return false;
    }

    // Check argument count
    if (Call.Callee != "print" && Call.Callee != "printi" &&
        Call.Args.size() != Fit->second.Params.size()) {
      Diags.error(E.Loc, "function '" + Call.Callee + "' expects " +
                             std::to_string(Fit->second.Params.size()) +
                             " arguments but got " +
                             std::to_string(Call.Args.size()));
      return false;
    }

    // Check argument types
    for (size_t I = 0; I < Call.Args.size(); I++) {
      if (!analyzeExpr(*Call.Args[I]))
        return false;
    }

    // Infer parameter types from call-site arguments
    for (size_t I = 0; I < Call.Args.size() && I < Fit->second.Params.size(); I++) {
      if (!Fit->second.Params[I].Type && Call.Args[I]->ResultType) {
        Fit->second.Params[I].Type = Call.Args[I]->ResultType->clone();
        InferredAny = true;
        // Also update AST so CodeGen uses correct types
        if (Fit->second.Decl && I < Fit->second.Decl->Params.size() &&
            !Fit->second.Decl->Params[I].ParamType) {
          Fit->second.Decl->Params[I].ParamType = Call.Args[I]->ResultType->clone();
        }
      }
    }

    setExprType(E, Fit->second.ReturnType->clone());
    return true;
  }
  case Expr::Kind::Assignment: {
    auto &Assign = static_cast<AssignmentExpr &>(E);
    VarInfo *V = CurrentScope->lookup(Assign.VarName);
    if (!V) {
      // Python-style: auto-declare on assignment
      if (!analyzeExpr(*Assign.Value))
        return false;
      VarInfo NewVar;
      NewVar.Name = Assign.VarName;
      NewVar.Type = Assign.Value->ResultType
                        ? Assign.Value->ResultType->clone()
                        : getBuiltinType(BuiltinTypeKind::I64);
      NewVar.IsMutable = true;
      NewVar.IsInitialized = true;
      if (!CurrentScope->addVar(std::move(NewVar))) {
        Diags.error(E.Loc, "cannot declare variable '" + Assign.VarName + "'");
        return false;
      }
      setExprType(E, nullptr);
      return true;
    }
    if (!V->IsMutable) {
      Diags.error(E.Loc, "assignment to immutable variable '" +
                              Assign.VarName + "'");
      return false;
    }
    if (!analyzeExpr(*Assign.Value))
      return false;
    setExprType(E, V->Type ? V->Type->clone() : nullptr);
    return true;
  }
  case Expr::Kind::MemberAccess:
  case Expr::Kind::ArrayIndex:
  case Expr::Kind::Cast:
    // Simplified - accept for now
    return true;
  }
  return true;
}
