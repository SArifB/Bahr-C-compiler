#pragma once
#include <lexer/mod.h>
#include <utility/mod.h>

typedef enum OperKind OperKind;
typedef enum TypeKind TypeKind;
typedef enum NodeKind NodeKind;
typedef struct Node Node;
typedef struct OperNode OperNode;
typedef struct VarCharArr VarCharArr;
typedef struct VariNode VariNode;
typedef struct ValueNode ValueNode;

enum OperKind {
  OP_Add,
  OP_Sub,
  OP_Mul,
  OP_Div,
  OP_Eq,
  OP_NEq,
  OP_Lt,
  OP_Lte,
  OP_Gte,
  OP_Gt,
};

struct OperNode {
  OperKind kind;
  Node* lhs;
  Node* rhs;
};

struct VarCharArr {
  usize size;
  char arr[];
};

enum TypeKind {
  TP_Int,
  TP_Flt,
  TP_Str,
};

struct VariNode {
  TypeKind kind;
  Node* next;
  VarCharArr name;
};

struct ValueNode {
  TypeKind kind;
  union {
    i64 i_num;
    f64 f_num;
    VarCharArr str_lit;
  };
};

enum NodeKind {
  // ND_Var,
  // ND_Call,
  ND_Oper,
  ND_Val,
  ND_Neg,
};

struct Node {
  NodeKind kind;
  union {
    OperNode binop;
    ValueNode value;
    Node* unaval;
  };
};

extern Node* parse_lexer(Token** rest, Token* token);
extern void print_ast_tree(Node* node);
extern void free_ast_tree();
