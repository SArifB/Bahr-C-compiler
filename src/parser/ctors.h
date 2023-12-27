#pragma once
#include <parser/mod.h>
#include <utility/vec.h>

extern Node* make_add(Node* lhs, Node* rhs, Token* token);
extern Node* make_sub(Node* lhs, Node* rhs, Token* token);
extern Node* make_oper(OperKind oper, Node* lhs, Node* rhs);
extern Node* make_unary(NodeKind kind, Node* value);
extern Node* make_basic_value(TypeKind type, StrView view);
extern Node* make_declaration(TypeKind type, StrView view, Node* value);
extern Node* make_if_node(Node* cond, Node* then, Node* elseb);
extern Node* make_while_node(Node* cond, Node* then);
extern Node* make_call_node(StrView view, Node* args);
extern Node* make_function(StrView view, Node* body, Node* args);

typedef struct Arena Arena;
extern Arena PARSER_ARENA;
extern any parser_alloc(usize size);

extern void free_none(any);

typedef struct Node* NodeRef;
DECLARE_VECTOR(NodeRef)

extern NodeRefVector* current_locals;
