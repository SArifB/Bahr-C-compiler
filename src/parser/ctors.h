#pragma once
#include <arena/mod.h>
#include <parser/mod.h>
#include <utility/vec.h>

// clang-format off
extern Node* make_add(Arena* arena, Node* lhs, Node* rhs, Token* token);
extern Node* make_sub(Arena* arena, Node* lhs, Node* rhs, Token* token);
extern Node* make_oper(Arena* arena, OperKind oper, Node* lhs, Node* rhs);
extern Node* make_unary(Arena* arena, NodeKind kind, Node* value);
extern Node* make_basic_value(Arena* arena, Node* type, StrView view);
extern Node* make_str_value(Arena* arena, StrView view);
extern Node* make_numeric_value(Arena* arena, Node* type, usize num);
extern Node* make_pointer_value(Arena* arena, Node* type, Node* value);
extern Node* make_basic_type(Arena* arena, TypeKind kind);
extern Node* make_numeric_type(Arena* arena, TypeKind kind, usize width);
extern Node* make_pointer_type(Arena* arena, Node* value);
extern Node* make_array_type(Arena* arena, Node* type, usize size);
extern Node* make_declaration(Context cx, Node* type, StrView view, Node* value);
extern Node* make_arg_var(Context cx, Node* type, StrView value);
extern Node* make_if_node(Arena* arena, Node* cond, Node* then, Node* elseb);
extern Node* make_while_node(Arena* arena, Node* cond, Node* then);
extern Node* make_call_node(Arena* arena, StrView view, Node* args);
extern Node* make_function(Arena* arena, Node* type, StrView view, Node* body, Node* args);
// clang-format on

extern void print_ast(Node* prog);
