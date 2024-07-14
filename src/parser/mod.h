#pragma once
#include <arena/mod.h>
#include <hashmap/mod.h>
#include <parser/lexer.h>
#include <utility/mod.h>

typedef struct OperNode OperNode;
typedef struct TypeNode TypeNode;
typedef struct ValueNode ValueNode;
typedef struct DeclNode DeclNode;
typedef struct FnNode FnNode;
typedef struct IfNode IfNode;
typedef struct WhileNode WhileNode;
typedef struct CallNode CallNode;
typedef struct Node Node;
typedef struct Context Context;
typedef struct ParserOptions ParserOptions;
typedef struct ParserOutput ParserOutput;

typedef HashMap* Scope;
DEFINE_VECTOR(Scope)

typedef enum {
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
} OperKind;

struct OperNode {
  OperKind kind;
  Node* lhs;
  Node* rhs;
};

typedef enum {
  TP_Undf,
  TP_Unit,
  TP_SInt,
  TP_UInt,
  TP_Flt,
  TP_Str,
  TP_Ptr,
  TP_Arr,
} TypeKind;

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
    usize number;
    StrArr* basic;
  };
};

struct DeclNode {
  Node* type;
  Node* value;
  StrArr* name;
};

typedef enum Linkage {
  LN_Private,
  LN_Public,
  LN_Unspecified,
} Linkage;

struct FnNode {
  Node* ret_type;
  Node* args;
  Node* body;
  StrArr* name;
  Linkage linkage;
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
  StrArr* name;
};

typedef enum {
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
} NodeKind;

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

struct Context {
  Arena* arena;
  ScopeVector* scopes;
  StrView input;
};

struct ParserOptions {
  StrView input;
  bool verbose;
};

struct ParserOutput {
  Node* tree;
  Arena arena;
};

extern ParserOutput parse_string(ParserOptions options);
