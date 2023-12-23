#pragma once
#include <parser/mod.h>

extern Node* make_add(Node* lhs, Node* rhs, Token* token);
extern Node* make_sub(Node* lhs, Node* rhs, Token* token);
extern Node* make_oper(OperKind oper, Node* lhs, Node* rhs);
extern Node* make_unary(NodeKind kind, Node* value);
extern Node* make_number(StrView view);
extern Node* make_variable(Object* obj);
extern Node* make_if_node(Node* cond, Node* then, Node* elseb);
extern Node* make_while_node(Node* cond, Node* then);
// extern Node* make_call_node(StrView view, Node* args);
extern Node* make_call_node(StrView view);
// extern Object* make_object(StrView view, Type* type);
extern Object* make_object(StrView view);
// extern Function* make_function(StrView view, Node* body, Object* args);
extern Function* make_function(StrView view, Node* body);


extern Object* current_locals;
