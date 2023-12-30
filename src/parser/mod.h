#pragma once
#include <lexer/mod.h>
#include <utility/mod.h>

typedef enum OperKind OperKind;
typedef enum TypeKind TypeKind;
typedef enum NodeKind NodeKind;
typedef struct VarCharArr VarCharArr;
typedef struct OperNode OperNode;
typedef struct ValueNode ValueNode;
typedef struct DeclNode DeclNode;
typedef struct FnNode FnNode;
typedef struct IfNode IfNode;
typedef struct WhileNode WhileNode;
typedef struct CallNode CallNode;
typedef struct NodeBase NodeBase;
typedef struct Node Node;

struct VarCharArr {
  usize size;
  char array[];
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

enum TypeKind {
  TP_Void,
  TP_I1,
  TP_I8,
  TP_I16,
  TP_I32,
  TP_I64,
  // TP_Flt,
  TP_Str,
  TP_Ptr,
};

struct ValueNode {
  TypeKind kind;
  Node* type;
  union {
    VarCharArr basic;
    Node* base;
  };
};

struct DeclNode {
  Node* type;
  Node* value;
  VarCharArr name;
};

typedef struct NodeRefVector NodeRefVector;
struct FnNode {
  Node* ret_type;
  Node* args;
  Node* body;
  NodeRefVector* locals;
  VarCharArr name;
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
  ND_Return,
  ND_Block,
  ND_Addr,
  ND_Deref,
  ND_Type,
  ND_Decl,
  ND_Value,
  ND_Variable,
  ND_ArgVar,
  ND_Function,
  ND_If,
  ND_While,
  ND_Call,
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
    Node* unary;
    ValueNode value;
    DeclNode declaration;
    FnNode function;
    IfNode if_node;
    WhileNode while_node;
    CallNode call_node;
  };
};

extern Node* parse_lexer(TokenVector* tokens);
extern void print_ast(Node* prog);
extern void free_ast();

#define view_from(var_arr) \
  ((StrView){(var_arr).array, (var_arr).array + (var_arr).size})
