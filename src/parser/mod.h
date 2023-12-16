#pragma once
#include <lexer/mod.h>
#include <utility/mod.h>

typedef enum OperKind OperKind;
typedef enum TypeKind TypeKind;
typedef enum NodeKind NodeKind;
typedef enum UnaryKind UnaryKind;
typedef struct Node Node;
typedef struct OperNode OperNode;
typedef struct VarCharArr VarCharArr;
typedef struct VarNode VarNode;
typedef struct ValueNode ValueNode;
typedef struct UnaryNode UnaryNode;

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
  OP_Asg,
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

struct VarNode {
  // TypeKind kind;
  // Node* next;
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

enum UnaryKind {
  UN_Negation,
  UN_ExprStmt,
  UN_Return,
};

struct UnaryNode {
  UnaryKind kind;
  Node* next;
};

enum NodeKind {
  ND_Operation,
  ND_Value,
  ND_Unary,
  ND_Variable,
};

struct Node {
  NodeKind kind;
  union {
    OperNode operation;
    ValueNode value;
    VarNode variable;
    UnaryNode unary;
  };
};

extern Node* parse_lexer(TokenVector* tokens);
extern void print_ast(Node* node);
extern void free_ast();
