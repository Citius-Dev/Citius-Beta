#include "CodeGen/CodeGen.h"
#include <cstdlib>
#include <cstdio>
#include <fstream>

//===----------------------------------------------------------------------===//
// Helpers
//===----------------------------------------------------------------------===//

std::string CodeGen::indent() {
  return std::string(IndentLevel * 2, ' ');
}

void CodeGen::emit(const std::string &Str) {
  Output << Str;
}

void CodeGen::emitLine(const std::string &Str) {
  Output << indent() << Str << "\n";
}

void CodeGen::emitInclude(const std::string &Header) {
  Output << "#include " << Header << "\n";
}

std::string CodeGen::getBuiltinCType(BuiltinTypeKind K) {
  switch (K) {
  case BuiltinTypeKind::Void:
    return "void";
  case BuiltinTypeKind::Bool:
    return "bool";
  case BuiltinTypeKind::I8:
    return "int8_t";
  case BuiltinTypeKind::I16:
    return "int16_t";
  case BuiltinTypeKind::I32:
    return "int32_t";
  case BuiltinTypeKind::I64:
    return "int64_t";
  case BuiltinTypeKind::U8:
    return "uint8_t";
  case BuiltinTypeKind::U16:
    return "uint16_t";
  case BuiltinTypeKind::U32:
    return "uint32_t";
  case BuiltinTypeKind::U64:
    return "uint64_t";
  case BuiltinTypeKind::F32:
    return "float";
  case BuiltinTypeKind::F64:
    return "double";
  }
  return "int64_t";
}

std::string CodeGen::getCTypeName(TypeNode *T) {
  if (!T)
    return "int64_t";

  if (auto *B = std::get_if<TypeNode::Builtin>(&T->Data))
    return getBuiltinCType(B->Kind);

  if (auto *N = std::get_if<TypeNode::Named>(&T->Data)) {
    if (N->Name == "str")
      return "char*";
    return N->Name;
  }

  if (auto *P = std::get_if<TypeNode::Pointer>(&T->Data))
    return getCTypeName(P->Pointee.get()) + "*";

  if (auto *A = std::get_if<TypeNode::Array>(&T->Data))
    return getCTypeName(A->ElemType.get()) + "*";

  return "int64_t";
}

std::string CodeGen::mangleType(TypeNode *T) {
  if (!T)
    return "i64";
  if (auto *B = std::get_if<TypeNode::Builtin>(&T->Data)) {
    switch (B->Kind) {
    case BuiltinTypeKind::Void:
      return "v";
    case BuiltinTypeKind::Bool:
      return "b";
    case BuiltinTypeKind::I8:
      return "i8";
    case BuiltinTypeKind::I16:
      return "i16";
    case BuiltinTypeKind::I32:
      return "i32";
    case BuiltinTypeKind::I64:
      return "i64";
    case BuiltinTypeKind::U8:
      return "u8";
    case BuiltinTypeKind::U16:
      return "u16";
    case BuiltinTypeKind::U32:
      return "u32";
    case BuiltinTypeKind::U64:
      return "u64";
    case BuiltinTypeKind::F32:
      return "f32";
    case BuiltinTypeKind::F64:
      return "f64";
    }
  }
  return "i64";
}

//===----------------------------------------------------------------------===//
// Expression code generation
//===----------------------------------------------------------------------===//

std::string CodeGen::genBinaryOp(BinaryOp Op, const std::string &LHS,
                                 const std::string &RHS, TypeNode *Ty) {
  bool IsFP = Ty && (getCTypeName(Ty) == "float" || getCTypeName(Ty) == "double");

  switch (Op) {
  case BinaryOp::Add:
    return "(" + LHS + " + " + RHS + ")";
  case BinaryOp::Sub:
    return "(" + LHS + " - " + RHS + ")";
  case BinaryOp::Mul:
    return "(" + LHS + " * " + RHS + ")";
  case BinaryOp::Div:
    return "(" + LHS + " / " + RHS + ")";
  case BinaryOp::Mod:
    return "(" + LHS + " % " + RHS + ")";
  case BinaryOp::Eq:
    return "(" + LHS + " == " + RHS + ")";
  case BinaryOp::Ne:
    return "(" + LHS + " != " + RHS + ")";
  case BinaryOp::Lt:
    return "(" + LHS + " < " + RHS + ")";
  case BinaryOp::Gt:
    return "(" + LHS + " > " + RHS + ")";
  case BinaryOp::Le:
    return "(" + LHS + " <= " + RHS + ")";
  case BinaryOp::Ge:
    return "(" + LHS + " >= " + RHS + ")";
  case BinaryOp::And:
    return "(" + LHS + " && " + RHS + ")";
  case BinaryOp::Or:
    return "(" + LHS + " || " + RHS + ")";
  case BinaryOp::BitAnd:
    return "(" + LHS + " & " + RHS + ")";
  case BinaryOp::BitOr:
    return "(" + LHS + " | " + RHS + ")";
  case BinaryOp::BitXor:
    return "(" + LHS + " ^ " + RHS + ")";
  case BinaryOp::Shl:
    return "(" + LHS + " << " + RHS + ")";
  case BinaryOp::Shr:
    return "(" + LHS + " >> " + RHS + ")";
  }
  return "(" + LHS + " + " + RHS + ")";
}

std::string CodeGen::genExpr(Expr &E) {
  switch (E.ExprKind) {
  case Expr::Kind::IntegerLit: {
    auto &Lit = static_cast<IntegerLiteralExpr &>(E);
    return std::to_string(Lit.Value);
  }
  case Expr::Kind::FloatLit: {
    auto &Lit = static_cast<FloatLiteralExpr &>(E);
    std::string S = std::to_string(Lit.Value);
    if (S.find('.') == std::string::npos && S.find('e') == std::string::npos)
      S += ".0";
    return S;
  }
  case Expr::Kind::StringLit: {
    auto &Lit = static_cast<StringLiteralExpr &>(E);
    std::string Escaped;
    for (char C : Lit.Value) {
      switch (C) {
      case '\n':
        Escaped += "\\n";
        break;
      case '\t':
        Escaped += "\\t";
        break;
      case '\\':
        Escaped += "\\\\";
        break;
      case '"':
        Escaped += "\\\"";
        break;
      default:
        Escaped += C;
      }
    }
    return "\"" + Escaped + "\"";
  }
  case Expr::Kind::CharLit: {
    auto &Lit = static_cast<CharLiteralExpr &>(E);
    return std::to_string(static_cast<int>(Lit.Value));
  }
  case Expr::Kind::BoolLit: {
    auto &Lit = static_cast<BoolLiteralExpr &>(E);
    return Lit.Value ? "1" : "0";
  }
  case Expr::Kind::Identifier: {
    auto &Id = static_cast<IdentifierExpr &>(E);
    auto It = VarScope.find(Id.Name);
    if (It != VarScope.end())
      return Id.Name;
    return Id.Name; // Could be a function name
  }
  case Expr::Kind::Binary: {
    auto &Bin = static_cast<BinaryExpr &>(E);
    std::string LHS = genExpr(*Bin.LHS);
    std::string RHS = genExpr(*Bin.RHS);
    TypeNode *Ty = E.ResultType.get();
    return genBinaryOp(Bin.Op, LHS, RHS, Ty);
  }
  case Expr::Kind::Unary: {
    auto &Un = static_cast<UnaryExpr &>(E);
    std::string Op = genExpr(*Un.Operand);
    switch (Un.Op) {
    case UnaryOp::Neg:
      return "(-" + Op + ")";
    case UnaryOp::Not:
      return "(!" + Op + ")";
    case UnaryOp::BitNot:
      return "(~" + Op + ")";
    }
    break;
  }
  case Expr::Kind::Call: {
    auto &Call = static_cast<CallExpr &>(E);
    if (Call.Callee == "print") {
      if (Call.Args.empty())
        return "printf(\"\")";
      auto *Arg = Call.Args[0].get();
      std::string Val = genExpr(*Arg);
      TypeNode *Ty = Arg->ResultType.get();
      std::string CTy = getCTypeName(Ty);

      if (CTy == "double" || CTy == "float")
        return "printf(\"%g\\n\", (double)(" + Val + "))";
      else if (CTy == "bool" || CTy == "_Bool")
        return "printf(\"%s\\n\", (" + Val + ") ? \"true\" : \"false\")";
      else if (CTy.find('*') != std::string::npos || CTy == "char*")
        return "printf(\"%s\\n\", " + Val + ")";
      else
        return "printf(\"%lld\\n\", (int64_t)(" + Val + "))";
    }
    if (Call.Callee == "printi") {
      if (Call.Args.empty())
        return "printf(\"\")";
      auto *Arg = Call.Args[0].get();
      std::string Val = genExpr(*Arg);
      return "printf(\"%lld\", (int64_t)(" + Val + "))";
    }

    std::string Code = Call.Callee + "(";
    for (size_t I = 0; I < Call.Args.size(); I++) {
      if (I > 0)
        Code += ", ";
      Code += genExpr(*Call.Args[I]);
    }
    Code += ")";
    return Code;
  }
  case Expr::Kind::Assignment: {
    auto &Assign = static_cast<AssignmentExpr &>(E);
    std::string Val = genExpr(*Assign.Value);
    return "(" + Assign.VarName + " = " + Val + ")";
  }
  case Expr::Kind::MemberAccess: {
    auto &MA = static_cast<MemberAccessExpr &>(E);
    return "(" + genExpr(*MA.Object) + "." + MA.Member + ")";
  }
  case Expr::Kind::ArrayIndex: {
    auto &AI = static_cast<ArrayIndexExpr &>(E);
    return "(" + genExpr(*AI.Array) + "[" + genExpr(*AI.Index) + "])";
  }
  case Expr::Kind::Cast:
    return "";
  }
  return "0";
}

//===----------------------------------------------------------------------===//
// Statement code generation
//===----------------------------------------------------------------------===//

void CodeGen::genStmt(Stmt &S) {
  switch (S.StmtKind) {
  case Stmt::Kind::VarDecl: {
    genVarDecl(static_cast<VarDeclStmt &>(S));
    break;
  }
  case Stmt::Kind::ExprStmt: {
    auto &ES = static_cast<ExprStmt &>(S);
    // Python-style auto-declare: `x = expr` at statement level
    if (ES.E->ExprKind == Expr::Kind::Assignment) {
      auto &Assign = static_cast<AssignmentExpr &>(*ES.E);
      if (VarScope.find(Assign.VarName) == VarScope.end()) {
        std::string Val = genExpr(*Assign.Value);
        std::string CTy = Assign.Value->ResultType
                              ? getCTypeName(Assign.Value->ResultType.get())
                              : "int64_t";
        VarScope[Assign.VarName] = {
            Assign.VarName,
            Assign.Value->ResultType
                ? Assign.Value->ResultType->clone()
                : nullptr,
            true};
        emitLine(CTy + " " + Assign.VarName + " = " + Val + ";");
        break;
      }
    }
    std::string Code = genExpr(*ES.E);
    if (!Code.empty())
      emitLine(Code + ";");
    break;
  }
  case Stmt::Kind::Block: {
    auto &B = static_cast<BlockStmt &>(S);
    emitLine("{");
    IndentLevel++;
    for (auto &St : B.Statements)
      genStmt(*St);
    IndentLevel--;
    emitLine("}");
    break;
  }
  case Stmt::Kind::If: {
    auto &IF = static_cast<IfStmt &>(S);
    std::string Cond = genExpr(*IF.Condition);
    emitLine("if (" + Cond + ")");
    genStmt(*IF.ThenBranch);
    if (IF.ElseBranch) {
      // Check if the else branch is another if (else if)
      if (IF.ElseBranch->StmtKind == Stmt::Kind::If) {
        emit("else ");
        genStmt(*IF.ElseBranch);
      } else {
        emitLine("else");
        genStmt(*IF.ElseBranch);
      }
    }
    break;
  }
  case Stmt::Kind::While: {
    auto &W = static_cast<WhileStmt &>(S);
    std::string Cond = genExpr(*W.Condition);
    emitLine("while (" + Cond + ")");
    genStmt(*W.Body);
    break;
  }
  case Stmt::Kind::For: {
    auto &F = static_cast<ForStmt &>(S);
    std::string Start = genExpr(*F.Start);
    std::string End = genExpr(*F.End);

    // Reserve local scope for for loop variable
    emitLine("{");
    IndentLevel++;
    emitLine("int64_t " + F.VarName + " = " + Start + ";");
    VarScope[F.VarName] = {F.VarName, nullptr, true};

    emitLine("for (; " + F.VarName + " < " + End + "; " + F.VarName + "++)");
    genStmt(*F.Body);

    VarScope.erase(F.VarName);
    IndentLevel--;
    emitLine("}");
    break;
  }
  case Stmt::Kind::Return: {
    auto &R = static_cast<ReturnStmt &>(S);
    if (R.Value) {
      std::string Val = genExpr(*R.Value);
      emitLine("return " + Val + ";");
    } else {
      emitLine("return;");
    }
    break;
  }
  case Stmt::Kind::Break: {
    emitLine("break;");
    break;
  }
  case Stmt::Kind::Continue: {
    emitLine("continue;");
    break;
  }
  case Stmt::Kind::Sleep: {
    genSleepStmt(static_cast<SleepStmt &>(S));
    break;
  }
  case Stmt::Kind::Sent: {
    genSentStmt(static_cast<SentStmt &>(S));
    break;
  }
  }
}

//===----------------------------------------------------------------------===//
// Variable declaration
//===----------------------------------------------------------------------===//

void CodeGen::genVarDecl(VarDeclStmt &VD) {
  std::string CTy = VD.DeclType ? getCTypeName(VD.DeclType.get()) : "int64_t";
  std::string Qual = VD.IsMutable ? "" : "const ";
  std::string Init = VD.Init ? genExpr(*VD.Init) : "0";

  std::string Decl = Qual + CTy + " " + VD.Name;
  if (VD.DeclType && std::holds_alternative<TypeNode::Array>(VD.DeclType->Data)) {
    // Array declaration - emit differently
    Decl = Qual + CTy + " " + VD.Name + "[] = {" + Init + "}";
  } else if (!Init.empty()) {
    Decl += " = " + Init;
  }
  emitLine(Decl + ";");

  VarScope[VD.Name] = {VD.Name,
                       VD.DeclType ? VD.DeclType->clone() : nullptr,
                       VD.IsMutable};
}

//===----------------------------------------------------------------------===//
// Sleep / Wait statement
//===----------------------------------------------------------------------===//

void CodeGen::genSleepStmt(SleepStmt &S) {
  NeedsSleep = true;
  std::string Dur = genExpr(*S.Duration);
  const char *Mult;
  switch (S.Unit) {
  case TokenKind::KwMs:    Mult = "1LL"; break;
  case TokenKind::KwSec:   Mult = "1000LL"; break;
  case TokenKind::KwMin:   Mult = "60000LL"; break;
  case TokenKind::KwHour:  Mult = "3600000LL"; break;
  case TokenKind::KwDay:   Mult = "86400000LL"; break;
  case TokenKind::KwMonth: Mult = "2592000000LL"; break;
  case TokenKind::KwYear:  Mult = "31536000000LL"; break;
  default:                 Mult = "1000LL"; break;
  }
  emitLine("citius_sleep((int64_t)(" + Dur + ") * " + Mult + ");");
}

//===----------------------------------------------------------------------===//
// Sent (network) statement
//===----------------------------------------------------------------------===//

void CodeGen::genSentStmt(SentStmt &S) {
  NeedsWinsock = true;
  emitLine("{");
  IndentLevel++;
  emitLine("citius_net_init();");

  if (S.SendMode == SentMode::Auto) {
    // Auto mode: HTTP request
    std::string Host = S.Address;
    std::string PortStr = S.Port ? std::to_string(S.Port) :
        (S.Proto == SentProtocol::HTTPS ? "443" : "80");
    std::string RequestEscaped;
    for (char C : S.RequestContent) {
      switch (C) {
      case '\n': RequestEscaped += "\\n"; break;
      case '\r': RequestEscaped += "\\r"; break;
      case '"':  RequestEscaped += "\\\""; break;
      case '\\': RequestEscaped += "\\\\"; break;
      default:   RequestEscaped += C;
      }
    }
    emitLine("citius_http_request(\"" + Host + "\", " + PortStr + ", \"" + RequestEscaped + "\""
             + ", " + (S.HasHeaders ? "1" : "0") + ", " + (S.HasPayload ? "1" : "0")
             + ", " + (S.IsRaw ? "1" : "0") + ");");
  } else {
    // Manual mode placeholder
    emitLine("printf(\"manual sent not yet implemented\\n\");");
  }

  IndentLevel--;
  emitLine("}");
}

//===----------------------------------------------------------------------===//
// Function
//===----------------------------------------------------------------------===//

void CodeGen::genFunction(FunctionDecl &Fn) {
  std::string RetTy = Fn.ReturnType ? getCTypeName(Fn.ReturnType.get()) : "void";
  std::string Vis = Fn.IsPub ? "" : "static ";

  Output << "\n";
  emitLine(Vis + RetTy + " " + Fn.Name + "(");

  // Parameters
  if (Fn.Params.empty()) {
    Output << "void";
  } else {
    IndentLevel++;
    for (size_t I = 0; I < Fn.Params.size(); I++) {
      std::string PTy = Fn.Params[I].ParamType
                            ? getCTypeName(Fn.Params[I].ParamType.get())
                            : "int64_t";
      std::string Decl = PTy + " " + Fn.Params[I].Name;
      if (I < Fn.Params.size() - 1)
        Decl += ",";
      emitLine(Decl);
    }
    IndentLevel--;
  }

  // Register params in scope
  for (auto &P : Fn.Params) {
    VarScope[P.Name] = {P.Name,
                        P.ParamType ? P.ParamType->clone() : nullptr, true};
  }

  emitLine(")");

  // Body
  emitLine("{");
  IndentLevel++;

  for (auto &St : Fn.Body->Statements)
    genStmt(*St);

  IndentLevel--;
  emitLine("}");
}

//===----------------------------------------------------------------------===//
// Main code generation
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Pre-scan: detect sleep/sent usage before codegen
//===----------------------------------------------------------------------===//

void CodeGen::scanStmt(Stmt *S) {
  if (!S) return;
  switch (S->StmtKind) {
  case Stmt::Kind::Sleep:
    NeedsSleep = true;
    break;
  case Stmt::Kind::Sent:
    NeedsWinsock = true;
    break;
  case Stmt::Kind::Block: {
    auto &B = static_cast<BlockStmt &>(*S);
    for (auto &St : B.Statements)
      scanStmt(St.get());
    break;
  }
  case Stmt::Kind::If: {
    auto &IF = static_cast<IfStmt &>(*S);
    scanStmt(IF.ThenBranch.get());
    scanStmt(IF.ElseBranch.get());
    break;
  }
  case Stmt::Kind::While: {
    auto &W = static_cast<WhileStmt &>(*S);
    scanStmt(W.Body.get());
    break;
  }
  case Stmt::Kind::For: {
    auto &F = static_cast<ForStmt &>(*S);
    scanStmt(F.Body.get());
    break;
  }
  default:
    break;
  }
}

bool CodeGen::generate(Program &Prog, std::string &OutStr) {
  // Reset
  Output.str("");
  Output.clear();
  VarScope.clear();
  IndentLevel = 0;
  NeedsWinsock = false;
  NeedsSleep = false;

  // Pre-scan all functions to detect sleep/sent usage
  for (auto &Fn : Prog.Functions) {
    scanStmt(Fn.Body.get());
  }

  // Generate header
  emitLine("// Generated by Citius Compiler");
  emitLine("#include <stdint.h>");
  emitLine("#include <stdbool.h>");
  emitLine("#include <stdio.h>");
  emitLine("#include <stdlib.h>");
  emitLine("#include <string.h>");
  emitLine("#include <math.h>");

  // Conditional platform includes
  if (NeedsSleep || NeedsWinsock) {
    emitLine("#ifdef _WIN32");
    emitLine("#include <windows.h>");
    emitLine("#endif");
  }
  if (NeedsWinsock) {
    emitLine("#ifdef _WIN32");
    emitLine("#include <winsock2.h>");
    emitLine("#include <ws2tcpip.h>");
    emitLine("#pragma comment(lib, \"ws2_32.lib\")");
    emitLine("#else");
    emitLine("#include <sys/socket.h>");
    emitLine("#include <netinet/in.h>");
    emitLine("#include <netdb.h>");
    emitLine("#include <arpa/inet.h>");
    emitLine("#include <unistd.h>");
    emitLine("typedef int SOCKET;");
    emitLine("#define INVALID_SOCKET (-1)");
    emitLine("#define closesocket close");
    emitLine("#endif");
  }

  // Emit sleep helper
  if (NeedsSleep) {
    emitSleepHelpers();
  }

  // Emit network helpers
  if (NeedsWinsock) {
    emitNetHelpers();
  }

  // Forward declarations
  for (auto &Fn : Prog.Functions) {
    std::string RetTy = Fn.ReturnType ? getCTypeName(Fn.ReturnType.get()) : "void";
    std::string Vis = Fn.IsPub ? "" : "static ";
    Output << Vis << RetTy << " " << Fn.Name << "(";
    if (Fn.Params.empty()) {
      Output << "void";
    } else {
      for (size_t I = 0; I < Fn.Params.size(); I++) {
        std::string PTy = Fn.Params[I].ParamType
                              ? getCTypeName(Fn.Params[I].ParamType.get())
                              : "int64_t";
        if (I > 0)
          Output << ", ";
        Output << PTy;
      }
    }
    Output << ");\n";
  }

  // Emit functions
  for (auto &Fn : Prog.Functions) {
    VarScope.clear();
    genFunction(Fn);
  }

  OutStr = Output.str();
  return true;
}

//===----------------------------------------------------------------------===//
// Helper emission
//===----------------------------------------------------------------------===//

void CodeGen::emitSleepHelpers() {
  emitLine("static void citius_sleep(int64_t ms) {");
  emitLine("#ifdef _WIN32");
  emitLine("  Sleep((DWORD)ms);");
  emitLine("#else");
  emitLine("  struct timespec ts;");
  emitLine("  ts.tv_sec = ms / 1000;");
  emitLine("  ts.tv_nsec = (long)((ms % 1000) * 1000000);");
  emitLine("  nanosleep(&ts, NULL);");
  emitLine("#endif");
  emitLine("}");
}

void CodeGen::emitNetHelpers() {
  emitLine("static int citius_net_inited = 0;");
  emitLine("static void citius_net_init(void) {");
  emitLine("  if (!citius_net_inited) {");
  emitLine("#ifdef _WIN32");
  emitLine("    WSADATA wsa;");
  emitLine("    WSAStartup(MAKEWORD(2,2), &wsa);");
  emitLine("#endif");
  emitLine("    citius_net_inited = 1;");
  emitLine("  }");
  emitLine("}");
  emitLine("");
  emitLine("static void citius_http_request(const char *host, int port,");
  emitLine("                                const char *request, int has_headers,");
  emitLine("                                int has_payload, int is_raw) {");
  emitLine("  SOCKET s = socket(AF_INET, SOCK_STREAM, 0);");
  emitLine("  if (s == INVALID_SOCKET) {");
  emitLine("    fprintf(stderr, \"socket error\\n\");");
  emitLine("    return;");
  emitLine("  }");
  emitLine("  struct hostent *he = gethostbyname(host);");
  emitLine("  if (!he) {");
  emitLine("    fprintf(stderr, \"host not found: %s\\n\", host);");
  emitLine("    closesocket(s);");
  emitLine("    return;");
  emitLine("  }");
  emitLine("  struct sockaddr_in addr;");
  emitLine("  memset(&addr, 0, sizeof(addr));");
  emitLine("  memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);");
  emitLine("  addr.sin_family = AF_INET;");
  emitLine("  addr.sin_port = htons((short)port);");
  emitLine("  if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) != 0) {");
  emitLine("    fprintf(stderr, \"connect failed\\n\");");
  emitLine("    closesocket(s);");
  emitLine("    return;");
  emitLine("  }");
  emitLine("  send(s, request, strlen(request), 0);");
  emitLine("  {");
  emitLine("    char buf[4096];");
  emitLine("    int n;");
  emitLine("    while ((n = recv(s, buf, sizeof(buf) - 1, 0)) > 0) {");
  emitLine("      buf[n] = 0;");
  emitLine("      printf(\"%s\", buf);");
  emitLine("    }");
  emitLine("  }");
  emitLine("  closesocket(s);");
  emitLine("}");
}

//===----------------------------------------------------------------------===//
// Compilation
//===----------------------------------------------------------------------===//

bool CodeGen::compileToExecutable(const std::string &SourcePath,
                                   const std::string &OutputPath,
                                   int OptLevel) {
  // Try clang first, then gcc, then MSVC cl
  std::string OptFlag = "-O" + std::to_string(OptLevel);
  std::string Cmd;

  Cmd = "clang " + SourcePath + " -o " + OutputPath + " " + OptFlag + " -lm -lws2_32 -std=c99 2>nul";
  int Ret = std::system(Cmd.c_str());
  if (Ret == 0)
    return true;

  Cmd = "gcc " + SourcePath + " -o " + OutputPath + " " + OptFlag + " -lm -lws2_32 -std=c99 2>nul";
  Ret = std::system(Cmd.c_str());
  if (Ret == 0)
    return true;

  Cmd = "cl.exe /nologo /Fe:" + OutputPath + " " + SourcePath + " /link ws2_32.lib /out:" + OutputPath + " 2>nul";
  Ret = std::system(Cmd.c_str());
  return Ret == 0;
}

bool CodeGen::compileToObject(const std::string &SourcePath,
                               const std::string &OutputPath, int OptLevel) {
  std::string OptFlag = "-O" + std::to_string(OptLevel);
  std::string Cmd;

  Cmd = "clang -c " + SourcePath + " -o " + OutputPath + " " + OptFlag + " -std=c99 2>nul";
  int Ret = std::system(Cmd.c_str());
  if (Ret == 0)
    return true;

  Cmd = "gcc -c " + SourcePath + " -o " + OutputPath + " " + OptFlag + " -std=c99 2>nul";
  Ret = std::system(Cmd.c_str());
  if (Ret == 0)
    return true;

  Cmd = "cl.exe /nologo /c " + SourcePath + " /Fo" + OutputPath + " 2>nul";
  Ret = std::system(Cmd.c_str());
  return Ret == 0;
}
