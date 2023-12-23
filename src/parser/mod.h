#pragma once
#include <lexer/mod.h>
#include <utility/mod.h>

typedef enum OperKind OperKind;
typedef enum TypeKind TypeKind;
typedef enum NodeKind NodeKind;
typedef struct VarCharArr VarCharArr;
typedef struct PtrType PtrType;
typedef struct FnType FnType;
typedef struct TypeBase TypeBase;
typedef struct Type Type;
typedef struct OperNode OperNode;
typedef struct ValueNode ValueNode;
typedef struct IfNode IfNode;
typedef struct WhileNode WhileNode;
typedef struct CallNode CallNode;
typedef struct NodeBase NodeBase;
typedef struct Node Node;
typedef struct Object Object;
typedef struct Function Function;

struct VarCharArr {
  usize size;
  char array[];
};

enum TypeKind {
  TP_Int,
  // TP_Flt,
  // TP_Str,
  TP_Ptr,
  TP_Fn,
};

struct PtrType {
  Type* base;
  VarCharArr name;
};

struct FnType {
  Type* args;
  Type* ret;
  VarCharArr name;
};

struct TypeBase {
  TypeKind kind;
  Type* next;
};

struct Type {
  union {
    struct {
      TypeKind kind;
      Type* next;
    };
    TypeBase base;
  };
  union {
    PtrType ptr_type;
    FnType fn_type;
    VarCharArr name;
  };
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

struct ValueNode {
  TypeKind kind;
  // union {
  i64 i_num;
  //   f64 f_num;
  //   VarCharArr str_lit;
  // };
};

struct IfNode {
  Node* cond;
  Node* then;
  Node* elseb;
};

struct WhileNode {
  Node* cond;
  Node* then;
};

struct CallNode {
  Node* args;
  VarCharArr name;
};

enum NodeKind {
  ND_None,
  ND_Operation,
  ND_Negation,
  ND_ExprStmt,
  ND_Return,
  ND_Block,
  ND_Addr,
  ND_Deref,
  ND_Value,
  ND_Variable,
  ND_If,
  ND_While,
  ND_Call,
};

struct NodeBase {
  NodeKind kind;
  Node* next;
  Type* type;
};

struct Node {
  union {
    struct {
      NodeKind kind;
      Node* next;
      Type* type;
    };
    NodeBase base;
  };
  union {
    OperNode operation;
    Node* unary;
    ValueNode value;
    Object* variable;
    IfNode if_node;
    WhileNode while_node;
    CallNode call_node;
  };
};

struct Object {
  Object* next;
  Type* type;
  VarCharArr name;
};

struct Function {
  Function* next;
  Object* args;
  Node* body;
  Object* locals;
  VarCharArr name;
};

extern Function* parse_lexer(TokenVector* tokens);
extern void print_ast(Function* prog);
extern void free_ast();

typedef struct Arena Arena;
extern Arena PARSER_ARENA;
extern any parser_alloc(usize size);
