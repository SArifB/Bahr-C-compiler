#pragma once
#include <parser/mod.h>
#include <utility/vec.h>

extern Node* make_add(Node* lhs, Node* rhs, Token* token);
extern Node* make_sub(Node* lhs, Node* rhs, Token* token);
extern Node* make_oper(OperKind oper, Node* lhs, Node* rhs);
extern Node* make_unary(NodeKind kind, Node* value);
extern Node* make_basic_value(Node* type, StrView view);
extern Node* make_str_value(StrView view);
extern Node* make_basic_type(TypeKind kind);
extern Node* make_numeric_type(TypeKind kind, i32 width);
extern Node* make_pointer_value(Node* type, Node* value);
extern Node* make_pointer_type(Node* value);
extern Node* make_declaration(Node* type, StrView view, Node* value);
extern Node* make_arg_var(Node* type, StrView view);
extern Node* make_if_node(Node* cond, Node* then, Node* elseb);
extern Node* make_while_node(Node* cond, Node* then);
extern Node* make_call_node(StrView view, Node* args);
extern Node* make_function(Node* type, StrView view, Node* body, Node* args);

extern fn(void*(usize)) parser_alloc;
extern fn(void(void*)) parser_dealloc;
extern NodeRefVector* current_locals;
