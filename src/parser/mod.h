#pragma once
#include <parser/lexer.h>
#include <utility/mod.h>

typedef enum OperKind OperKind;
typedef enum TypeKind TypeKind;
typedef enum NodeKind NodeKind;
typedef struct OperNode OperNode;
typedef struct TypeNode TypeNode;
typedef struct ValueNode ValueNode;
typedef struct DeclNode DeclNode;
typedef struct FnNode FnNode;
typedef struct IfNode IfNode;
typedef struct WhileNode WhileNode;
typedef struct CallNode CallNode;
typedef struct Node Node, *NodeRef;
DECLARE_VECTOR(NodeRef)

enum OperKind {
  OP_Add,
  // OP_PtrAdd,
  OP_Sub,
  // OP_PtrSub,
  // OP_PtrPtrSub,
  OP_Mul,
  OP_Div,
  OP_Eq,
  OP_NEq,
  OP_Lt,
  OP_Lte,
  OP_Gte,
  OP_Gt,
  OP_Asg,
  OP_ArrIdx,
};

struct OperNode {
  OperKind kind;
  Node* lhs;
  Node* rhs;
};

enum TypeKind {
  TP_Undf,
  TP_Unit,
  TP_SInt,
  TP_UInt,
  TP_Flt,
  TP_Str,
  TP_Ptr,
  TP_Arr,
};

struct TypeNode {
  TypeKind kind;
  union {
    usize bit_width;
    Node* base;
    struct {
      Node* base;
      usize size;
    } array;
  };
};

struct ValueNode {
  Node* type;
  union {
    Node* base;
    u64 number;
    StrSpan basic;
  };
};

struct DeclNode {
  Node* type;
  Node* value;
  StrSpan name;
};

struct FnNode {
  Node* ret_type;
  Node* args;
  Node* body;
  NodeRefVector* locals;
  StrSpan name;
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
  StrSpan name;
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

#define NODE_BASE_SIZE (sizeof(usize) * 2)

struct Node {
  NodeKind kind;
  Node* next;
  union {
    OperNode operation;
    Node* unary;
    TypeNode type;
    ValueNode value;
    DeclNode declaration;
    FnNode function;
    IfNode if_node;
    WhileNode while_node;
    CallNode call_node;
  };
};

extern Node* parse_string(const StrView view);
extern void enable_verbosity(bool enable);

extern void parser_set_alloc(fn(void*(usize)) ctor);
extern void parser_set_dealloc(fn(void(void*)) dtor);

#define view_from(var_arr) \
  ((StrView){(var_arr).array, (var_arr).array + (var_arr).size})
