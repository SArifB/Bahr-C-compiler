#pragma once
#include "utility/vec.h"
#include <lexer/mod.h>
#include <utility/mod.h>

typedef enum OperKind OperKind;
enum OperKind {
  OP_Plus,
  OP_Minus,
  OP_Mul,
  OP_Div,
  OP_Neg,
  OP_Lt,
  OP_Lte,
  OP_Eq,
  OP_Gte,
  OP_Gt,
  OP_Not,
};

typedef enum TypeKind TypeKind;
enum TypeKind {
  TP_Int,
  TP_Flt,
  TP_Str,
};

typedef enum NodeKind NodeKind;
enum NodeKind {
  ND_Var,
  // ND_Call,
  ND_Oper,
  ND_Val,
};

typedef struct Node Node;

typedef struct OperNode OperNode;
struct OperNode {
  OperKind kind;
  Node* lhs;
  Node* rhs;
};

typedef struct VariNode VariNode;
struct VariNode {
  TypeKind kind;
  cstr name;
  Node* next;
};

typedef struct ValueNode ValueNode;
struct ValueNode {
  TypeKind kind;
  union {
    i64 i_num;
    f64 f_num;
    StrView str_lit;
  };
};

struct Node {
  NodeKind kind;
  union {
    OperNode binop;
    ValueNode value;
  };
};
DECLARE_VECTOR(Node)

extern Node* expr(Token** rest, Token* tok);
extern Node* mul(Token** rest, Token* tok);
extern Node* primary(Token** rest, Token* tok);
extern void print_ast_tree(Node* node);
extern void free_ast_tree(Node* node);

/*
pub enum Expression<'a> {
Value(&'a str, TypeKind),
Scope {
name: &'a str,
args: NextExprArr<'a>,
body: NextExpr<'a>,
then: NextExpr<'a>,
},
TypeScope {
name: &'a str,
vars: NextExprArr<'a>,
},
Call(&'a str, NextExprArr<'a>),
Minus(NextExpr<'a>),
Operation(OperationType, NextExpr<'a>, NextExpr<'a>),
Order(OrderType, NextExpr<'a>, NextExpr<'a>),
}
*/
/*
typedef enum ExprKind ExprKind;
typedef enum OperKind OperKind;
typedef enum TypeKind TypeKind;
typedef struct VariExpr VariExpr;
typedef struct CallExpr CallExpr;
typedef struct BinExpr BinExpr;
typedef struct ValExpr ValExpr;
typedef struct CondExpr CondExpr;
typedef struct LoopExpr LoopExpr;
typedef struct Expr Expr;
typedef struct Expr Expr;
typedef enum TopNodeKind TopNodeKind;
typedef struct Prototype Prototype;
typedef struct Function Function;
typedef struct TopNode TopNode;
DECLARE_VECTOR(str);

enum OperKind {
  OP_Plus,
  OP_Minus,
  OP_Mul,
  OP_Div,
  OP_Neg,
  OP_Lt,
  OP_Lte,
  OP_Eq,
  OP_Gte,
  OP_Gt,
  OP_Not,
};

enum TypeKind {
  TP_Int,
  TP_Flt,
  TP_Str,
};

enum ExprKind {
  EX_Var,
  EX_Call,
  EX_Binr,
  EX_Val,
};

struct VariExpr {
  TypeKind type_kind;
  StrView name;
  Expr* value;
};

struct ValExpr {
  union {
    i64 int_num;
    f64 fp_num;
    StrView string;
  };
};

struct CallExpr {
  StrView name;
  Expr* args;
};

struct BinExpr {
  OperKind kind;
  Expr* left;
  Expr* right;
};

struct CondExpr {
  Expr* cond;
  Expr* then;
  Expr* alter;
};

struct LoopExpr {
  cstr name;
  Expr* start;
  Expr* end;
  Expr* step;
  Expr* body;
};

struct Expr {
  ExprKind kind;
  union {
    VariExpr vari;
    ValExpr value;
    CallExpr call;
    BinExpr binop;
    CondExpr cond;
    LoopExpr loop;
  };
};

enum TopNodeKind {
  TN_Extern,
  TN_Function,
  TN_Expression,
};

struct Prototype {
  cstr name;
  strVector* args;
};

struct Function {
  Prototype prot;
  Expr* body;
};

struct TopNode {
  TopNodeKind kind;
  union {
    Prototype* extern_proto;
    Function* function;
    Function* expr;
  };
};
DECLARE_VECTOR(TopNode);

TopNodeVector* parse_lexer(TokenVector* tokens);
 */
// -----------------------------------------------------------------------------------------
/*
typedef enum ExprAstKind ExprAstKind;
typedef struct ExprAst ExprAst;
typedef enum BinExprKind BinExprKind;
typedef struct BinExprAst BinExprAst;
typedef struct CallExprAst CallExprAst;
typedef enum TopNodeKind TopNodeKind;
typedef struct TopNode TopNode;
DECLARE_VECTOR(char);
DECLARE_VECTOR(str);
typedef struct ProtAst ProtAst;
typedef struct FuncAst FuncAst;

enum BinExprKind {
  BNX_Plus,
  BNX_Minus,
  BNX_Mul,
  BNX_Div,
};

struct BinExprAst {
  BinExprKind op;
  ExprAst* lhs;
  ExprAst* rhs;
};

struct CallExprAst {
  StrView callee;
  charVector args;
};

enum ExprAstKind {
  EXA_Num,
  EXA_Var,
  EXA_Bin,
  EXA_Call,
};

struct ExprAst {
  ExprAstKind type;
  union {
    double number_val;
    charVector variable;
    BinExprAst binary_expr;
    CallExprAst call_expr;
  };
};

enum TopNodeKind {
  TPN_Extern,
  TPN_Func,
  TPN_Expr,
};

struct ProtAst {
  StrView name;
  strVector args;
};

struct FuncAst {
  ExprAst* body;
  ProtAst* proto;
};

struct TopNode {
  TopNodeKind type;
  union {
    ProtAst* extern_proto;
    FuncAst* function;
    FuncAst* expr;
  };
};
DECLARE_VECTOR(TopNode);

// extern TopNodeVector root_node;
 */