#pragma once
#include <lexer/mod.h>
#include <utility/mod.h>

typedef enum OperKind OperKind;
typedef enum UnaryKind UnaryKind;
typedef enum TypeKind TypeKind;
typedef enum NodeKind NodeKind;
typedef struct VarCharArr VarCharArr;
typedef struct OperNode OperNode;
typedef struct UnaryNode UnaryNode;
typedef struct ValueNode ValueNode;
typedef struct IfNode IfNode;
typedef struct NodeBase NodeBase;
typedef struct Node Node;
typedef struct Object Object;
typedef struct Function Function;

struct VarCharArr {
  usize size;
  char arr[];
};

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

enum UnaryKind {
  UN_Negation,
  UN_ExprStmt,
  UN_Return,
};

struct UnaryNode {
  UnaryKind kind;
  Node* next;
};

enum TypeKind {
  TP_Int,
  TP_Flt,
  TP_Str,
};

struct ValueNode {
  TypeKind kind;
  union {
    i64 i_num;
    f64 f_num;
    VarCharArr str_lit;
  };
};

struct IfNode {
  Node* cond;
  Node* then;
  Node* elseb;
};

enum NodeKind {
  ND_None,
  ND_Operation,
  ND_Unary,
  ND_Value,
  ND_Variable,
  ND_Block,
  ND_If,
};

struct NodeBase {
  NodeKind kind;
  Node* next;
};

struct Node {
  union {
    struct {
      NodeKind kind;
      Node* next;
    };
    NodeBase base;
  };
  union {
    OperNode operation;
    UnaryNode unary;
    ValueNode value;
    Object* variable;
    Node* block;
    IfNode ifblock;
  };
};

struct Object {
  Object* next;
  VarCharArr name;
};

struct Function {
  Node* body;
  Object* locals;
};

extern Function* parse_lexer(TokenVector* tokens);
extern void print_ast(Function* prog);
extern void free_ast();
