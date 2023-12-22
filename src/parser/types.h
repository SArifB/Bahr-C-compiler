#pragma once
#include <parser/mod.h>

extern bool is_numeric(Type* type);
extern void add_type(Node* node);
extern Type* copy_type(Type* type);
extern Type* make_ptr_type(Type* base);
extern Type* make_decl_type(TypeKind kind);
extern Type* make_fn_type(Type* args, Type* ret);
