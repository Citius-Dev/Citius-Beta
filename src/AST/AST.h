#ifndef CITIUS_AST_H
#define CITIUS_AST_H

#include "Basic/Token.h"
#include "Basic/SourceLocation.h"
#include "Basic/TokenKinds.h"
#include <memory>
#include <string>
#include <vector>
#include <variant>

//===----------------------------------------------------------------------===//
// Type representation
//===----------------------------------------------------------------------===//

enum class BuiltinTypeKind {
  Void,
  Bool,
  I8, I16, I32, I64,
  U8, U16, U32, U64,
  F32, F64,
};

struct TypeNode {
  struct Named { std::string Name; };
  struct Builtin { BuiltinTypeKind Kind; };
  struct Pointer { std::unique_ptr<TypeNode> Pointee; bool IsMut; };
  struct Array { std::unique_ptr<TypeNode> ElemType; std::unique_ptr<class Expr> Size; };

  using TypeData = std::variant<Named, Builtin, Pointer, Array>;
  TypeData Data;
  SourceLocation Loc;

  explicit TypeNode(TypeData Data, SourceLocation Loc = {})
      : Data(std::move(Data)), Loc(Loc) {}

  static std::unique_ptr<TypeNode> createVoid(SourceLocation Loc = {}) {
    return std::make_unique<TypeNode>(Builtin{BuiltinTypeKind::Void}, Loc);
  }
  static std::unique_ptr<TypeNode> createBool(SourceLocation Loc = {}) {
    return std::make_unique<TypeNode>(Builtin{BuiltinTypeKind::Bool}, Loc);
  }
  static std::unique_ptr<TypeNode> createI32(SourceLocation Loc = {}) {
    return std::make_unique<TypeNode>(Builtin{BuiltinTypeKind::I32}, Loc);
  }
  static std::unique_ptr<TypeNode> createF64(SourceLocation Loc = {}) {
    return std::make_unique<TypeNode>(Builtin{BuiltinTypeKind::F64}, Loc);
  }

  std::unique_ptr<TypeNode> clone() const {
    return std::visit(
        [this](auto &&Arg) -> std::unique_ptr<TypeNode> {
          using T = std::decay_t<decltype(Arg)>;
          if constexpr (std::is_same_v<T, Named>) {
            return std::make_unique<TypeNode>(Named{Arg.Name}, Loc);
          } else if constexpr (std::is_same_v<T, Builtin>) {
            return std::make_unique<TypeNode>(Builtin{Arg.Kind}, Loc);
          } else if constexpr (std::is_same_v<T, Pointer>) {
            return std::make_unique<TypeNode>(
                Pointer{Arg.Pointee ? Arg.Pointee->clone() : nullptr,
                        Arg.IsMut},
                Loc);
          } else if constexpr (std::is_same_v<T, Array>) {
            return std::make_unique<TypeNode>(
                Array{Arg.ElemType ? Arg.ElemType->clone() : nullptr, nullptr},
                Loc);
          }
          return std::make_unique<TypeNode>(Named{""}, Loc);
        },
        Data);
  }
};

//===----------------------------------------------------------------------===//
// Expressions
//===----------------------------------------------------------------------===//

class Expr {
public:
  enum class Kind {
    IntegerLit,
    FloatLit,
    StringLit,
    CharLit,
    BoolLit,
    Identifier,
    Binary,
    Unary,
    Call,
    Assignment,
    MemberAccess,
    ArrayIndex,
    Cast,
  };

  Kind ExprKind;
  SourceLocation Loc;
  std::unique_ptr<TypeNode> ResultType;

  Expr(Kind K, SourceLocation Loc) : ExprKind(K), Loc(Loc) {}
  virtual ~Expr() = default;
};

struct IntegerLiteralExpr : public Expr {
  int64_t Value;
  IntegerLiteralExpr(int64_t V, SourceLocation Loc)
      : Expr(Kind::IntegerLit, Loc), Value(V) {}
};

struct FloatLiteralExpr : public Expr {
  double Value;
  FloatLiteralExpr(double V, SourceLocation Loc)
      : Expr(Kind::FloatLit, Loc), Value(V) {}
};

struct StringLiteralExpr : public Expr {
  std::string Value;
  StringLiteralExpr(std::string V, SourceLocation Loc)
      : Expr(Kind::StringLit, Loc), Value(std::move(V)) {}
};

struct CharLiteralExpr : public Expr {
  char Value;
  CharLiteralExpr(char V, SourceLocation Loc)
      : Expr(Kind::CharLit, Loc), Value(V) {}
};

struct BoolLiteralExpr : public Expr {
  bool Value;
  BoolLiteralExpr(bool V, SourceLocation Loc)
      : Expr(Kind::BoolLit, Loc), Value(V) {}
};

struct IdentifierExpr : public Expr {
  std::string Name;
  IdentifierExpr(std::string Name, SourceLocation Loc)
      : Expr(Kind::Identifier, Loc), Name(std::move(Name)) {}
};

enum class BinaryOp {
  Add, Sub, Mul, Div, Mod,
  Eq, Ne, Lt, Gt, Le, Ge,
  And, Or,
  BitAnd, BitOr, BitXor,
  Shl, Shr,
};

struct BinaryExpr : public Expr {
  BinaryOp Op;
  std::unique_ptr<Expr> LHS;
  std::unique_ptr<Expr> RHS;
  BinaryExpr(BinaryOp Op, std::unique_ptr<Expr> LHS,
             std::unique_ptr<Expr> RHS, SourceLocation Loc)
      : Expr(Kind::Binary, Loc), Op(Op), LHS(std::move(LHS)),
        RHS(std::move(RHS)) {}
};

enum class UnaryOp {
  Neg, Not, BitNot,
};

struct UnaryExpr : public Expr {
  UnaryOp Op;
  std::unique_ptr<Expr> Operand;
  UnaryExpr(UnaryOp Op, std::unique_ptr<Expr> Operand, SourceLocation Loc)
      : Expr(Kind::Unary, Loc), Op(Op), Operand(std::move(Operand)) {}
};

struct CallExpr : public Expr {
  std::string Callee;
  std::vector<std::unique_ptr<Expr>> Args;
  CallExpr(std::string Callee, std::vector<std::unique_ptr<Expr>> Args,
           SourceLocation Loc)
      : Expr(Kind::Call, Loc), Callee(std::move(Callee)),
        Args(std::move(Args)) {}
};

struct AssignmentExpr : public Expr {
  std::string VarName;
  std::unique_ptr<Expr> Value;
  AssignmentExpr(std::string VarName, std::unique_ptr<Expr> Value,
                 SourceLocation Loc)
      : Expr(Kind::Assignment, Loc), VarName(std::move(VarName)),
        Value(std::move(Value)) {}
};

struct MemberAccessExpr : public Expr {
  std::unique_ptr<Expr> Object;
  std::string Member;
  MemberAccessExpr(std::unique_ptr<Expr> Object, std::string Member,
                   SourceLocation Loc)
      : Expr(Kind::MemberAccess, Loc), Object(std::move(Object)),
        Member(std::move(Member)) {}
};

struct ArrayIndexExpr : public Expr {
  std::unique_ptr<Expr> Array;
  std::unique_ptr<Expr> Index;
  ArrayIndexExpr(std::unique_ptr<Expr> Array, std::unique_ptr<Expr> Index,
                 SourceLocation Loc)
      : Expr(Kind::ArrayIndex, Loc), Array(std::move(Array)),
        Index(std::move(Index)) {}
};

//===----------------------------------------------------------------------===//
// Statements
//===----------------------------------------------------------------------===//

class Stmt {
public:
  enum class Kind {
    VarDecl,
    ExprStmt,
    Block,
    If,
    While,
    For,
    Return,
    Break,
    Continue,
    Sleep,       // sleep/wait delay
    Sent,        // network request
  };

  Kind StmtKind;
  SourceLocation Loc;

  Stmt(Kind K, SourceLocation Loc) : StmtKind(K), Loc(Loc) {}
  virtual ~Stmt() = default;
};

struct VarDeclStmt : public Stmt {
  bool IsMutable;
  bool IsPub;
  std::string Name;
  std::unique_ptr<TypeNode> DeclType;
  std::unique_ptr<Expr> Init;

  VarDeclStmt(bool IsMutable, bool IsPub, std::string Name,
              std::unique_ptr<TypeNode> DeclType, std::unique_ptr<Expr> Init,
              SourceLocation Loc)
      : Stmt(Kind::VarDecl, Loc), IsMutable(IsMutable), IsPub(IsPub),
        Name(std::move(Name)), DeclType(std::move(DeclType)),
        Init(std::move(Init)) {}
};

struct ExprStmt : public Stmt {
  std::unique_ptr<Expr> E;
  ExprStmt(std::unique_ptr<Expr> E, SourceLocation Loc)
      : Stmt(Kind::ExprStmt, Loc), E(std::move(E)) {}
};

struct BlockStmt : public Stmt {
  std::vector<std::unique_ptr<Stmt>> Statements;
  BlockStmt(std::vector<std::unique_ptr<Stmt>> Statements,
            SourceLocation Loc)
      : Stmt(Kind::Block, Loc), Statements(std::move(Statements)) {}
};

struct IfStmt : public Stmt {
  std::unique_ptr<Expr> Condition;
  std::unique_ptr<Stmt> ThenBranch;
  std::unique_ptr<Stmt> ElseBranch;
  IfStmt(std::unique_ptr<Expr> Condition, std::unique_ptr<Stmt> ThenBranch,
         std::unique_ptr<Stmt> ElseBranch, SourceLocation Loc)
      : Stmt(Kind::If, Loc), Condition(std::move(Condition)),
        ThenBranch(std::move(ThenBranch)),
        ElseBranch(std::move(ElseBranch)) {}
};

struct WhileStmt : public Stmt {
  std::unique_ptr<Expr> Condition;
  std::unique_ptr<Stmt> Body;
  WhileStmt(std::unique_ptr<Expr> Condition, std::unique_ptr<Stmt> Body,
            SourceLocation Loc)
      : Stmt(Kind::While, Loc), Condition(std::move(Condition)),
        Body(std::move(Body)) {}
};

struct ForStmt : public Stmt {
  std::string VarName;
  std::unique_ptr<Expr> Start;
  std::unique_ptr<Expr> End;
  std::unique_ptr<Stmt> Body;
  ForStmt(std::string VarName, std::unique_ptr<Expr> Start,
          std::unique_ptr<Expr> End, std::unique_ptr<Stmt> Body,
          SourceLocation Loc)
      : Stmt(Kind::For, Loc), VarName(std::move(VarName)),
        Start(std::move(Start)), End(std::move(End)),
        Body(std::move(Body)) {}
};

struct ReturnStmt : public Stmt {
  std::unique_ptr<Expr> Value;
  ReturnStmt(std::unique_ptr<Expr> Value, SourceLocation Loc)
      : Stmt(Kind::Return, Loc), Value(std::move(Value)) {}
};

struct BreakStmt : public Stmt {
  BreakStmt(SourceLocation Loc) : Stmt(Kind::Break, Loc) {}
};

struct ContinueStmt : public Stmt {
  ContinueStmt(SourceLocation Loc) : Stmt(Kind::Continue, Loc) {}
};

//===----------------------------------------------------------------------===//
// Sleep / Wait (delay) statement
//===----------------------------------------------------------------------===//

struct SleepStmt : public Stmt {
  std::unique_ptr<Expr> Duration;  // numeric value
  TokenKind Unit;                  // KwMs, KwSec, KwMin, KwHour, KwDay, KwMonth, KwYear
  bool IsWait;                     // false = sleep, true = wait

  SleepStmt(std::unique_ptr<Expr> Duration, TokenKind Unit, bool IsWait,
            SourceLocation Loc)
      : Stmt(Kind::Sleep, Loc), Duration(std::move(Duration)), Unit(Unit),
        IsWait(IsWait) {}
};

//===----------------------------------------------------------------------===//
// Sent (network request) statement
//===----------------------------------------------------------------------===//

enum class SentProtocol { HTTP, HTTPS, TCP, UDP };
enum class SentMode { Auto, Manual };
enum class SentManualType { Hex, Binary, Text };

struct SentStmt : public Stmt {
  SentProtocol Proto;
  SentMode SendMode;

  // Auto mode fields
  std::string RequestContent;

  // Manual mode fields
  SentManualType ManType;
  std::string ManualData;

  // Target address
  std::string Address;
  uint16_t Port;  // 0 = use protocol default

  // Headers (auto mode only)
  std::vector<std::pair<std::string, std::string>> Headers;
  bool HasHeaders;  // false = auto-inject defaults

  // Payload (auto mode only)
  std::unique_ptr<Expr> Payload;
  bool HasPayload;

  // Raw mode: suppress all default headers
  bool IsRaw;

  SentStmt(SentProtocol Proto, SentMode SendMode,
           std::string RequestContent,
           SentManualType ManType, std::string ManualData,
           std::string Address, uint16_t Port,
           std::vector<std::pair<std::string, std::string>> Headers,
           bool HasHeaders,
           std::unique_ptr<Expr> Payload, bool HasPayload,
           bool IsRaw, SourceLocation Loc)
      : Stmt(Kind::Sent, Loc), Proto(Proto), SendMode(SendMode),
        RequestContent(std::move(RequestContent)),
        ManType(ManType), ManualData(std::move(ManualData)),
        Address(std::move(Address)), Port(Port),
        Headers(std::move(Headers)), HasHeaders(HasHeaders),
        Payload(std::move(Payload)), HasPayload(HasPayload),
        IsRaw(IsRaw) {}
};

//===----------------------------------------------------------------------===//
// Top-level declarations
//===----------------------------------------------------------------------===//

struct ParamDecl {
  std::string Name;
  std::unique_ptr<TypeNode> ParamType;
  SourceLocation Loc;

  ParamDecl(std::string Name, std::unique_ptr<TypeNode> ParamType,
            SourceLocation Loc)
      : Name(std::move(Name)), ParamType(std::move(ParamType)), Loc(Loc) {}
};

struct FunctionDecl {
  bool IsPub;
  std::string Name;
  std::vector<ParamDecl> Params;
  std::unique_ptr<TypeNode> ReturnType;
  std::unique_ptr<BlockStmt> Body;
  SourceLocation Loc;

  FunctionDecl(bool IsPub, std::string Name, std::vector<ParamDecl> Params,
               std::unique_ptr<TypeNode> ReturnType,
               std::unique_ptr<BlockStmt> Body, SourceLocation Loc)
      : IsPub(IsPub), Name(std::move(Name)), Params(std::move(Params)),
        ReturnType(std::move(ReturnType)), Body(std::move(Body)), Loc(Loc) {}
};

struct ImportDecl {
  std::string ModuleName;
  SourceLocation Loc;
};

struct OtherhaveDecl {
  std::vector<std::string> Languages;
  SourceLocation Loc;

  OtherhaveDecl(std::vector<std::string> Languages, SourceLocation Loc)
      : Languages(std::move(Languages)), Loc(Loc) {}
};

struct AnnotateDecl {
  std::string Symbol1;
  std::string Symbol2;
  std::string Type;  // "br" or "in"
  SourceLocation Loc;

  AnnotateDecl(std::string Symbol1, std::string Symbol2,
               std::string Type, SourceLocation Loc)
      : Symbol1(std::move(Symbol1)), Symbol2(std::move(Symbol2)),
        Type(std::move(Type)), Loc(Loc) {}
};

struct PackageDecl {
  std::string Name;
  SourceLocation Loc;

  PackageDecl(std::string Name, SourceLocation Loc)
      : Name(std::move(Name)), Loc(Loc) {}
};

struct StructMember {
  std::string Name;
  std::unique_ptr<TypeNode> MemberType;
  bool IsPub;
};

struct StructDecl {
  std::string Name;
  std::vector<StructMember> Members;
  bool IsPub{false};
  SourceLocation Loc;
};

struct Program {
  std::vector<ImportDecl> Imports;
  std::vector<FunctionDecl> Functions;
  std::vector<StructDecl> Structs;
  // New top-level declarations
  std::unique_ptr<OtherhaveDecl> Otherhave;
  std::unique_ptr<AnnotateDecl> Annotate;
  std::unique_ptr<PackageDecl> Package;
};

#endif
