#pragma once
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

extern Node* expr(Token** rest, Token* tok);
extern Node* mul(Token** rest, Token* tok);
extern Node* primary(Token** rest, Token* tok);
extern void print_ast_tree(Node* node);
extern void free_ast_tree();

