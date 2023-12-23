#include <lexer/errors.h>
#include <parser/ctors.h>
#include <parser/mod.h>
#include <parser/types.h>
#include <stdlib.h>
#include <string.h>

bool is_numeric(Type* type) {
  return type->kind == TP_Int;
}

void add_type(Node* node) {
  if (node == nullptr || node->type != nullptr) {
    // return;

  } else if (node->kind == ND_Operation) {
    add_type(node->operation.lhs);
    add_type(node->operation.rhs);
    if (
      node->kind == OP_Add || node->kind == OP_Sub || node->kind == OP_Mul || //
      node->kind == OP_Div || node->kind == OP_Asg
    ) {
      node->type = node->operation.lhs->type;
    } else {
      node->type = make_decl_type(TP_Int);
    }
  } else if (node->kind == ND_If) {
    add_type(node->if_node.cond);
    add_type(node->if_node.then);
    add_type(node->if_node.elseb);

  } else if (node->kind == ND_While) {
    add_type(node->while_node.cond);
    add_type(node->while_node.then);

  } else if (node->kind == ND_Call) {
    add_type(node->call_node.args);

  } else if (node->kind == ND_Negation) {
    node->type = node->unary->type;

  } else if (node->kind == ND_Variable) {
    node->type = node->variable->type;

  } else if (node->kind == ND_Addr) {
    node->type = make_ptr_type(node->unary->type);

  } else if (node->kind == ND_Deref) {
    if (node->operation.lhs->type->kind != TP_Ptr) {
      error("Invalid pointer dereference");
    } else {
      node->type = node->operation.lhs->type->ptr_type.base;
    }
  } else {
    for (Node* branch = node->unary; branch != nullptr; branch = branch->next) {
      add_type(branch);
    }
  }
}

Type* copy_type(Type* type) {
  if (type->kind == TP_Ptr) {
    usize str_size = sizeof(char) * type->ptr_type.name.size;
    Type* tmp = parser_alloc(sizeof(TypeBase) + sizeof(PtrType) + str_size);
    strncpy((str)tmp, (str)type, sizeof(TypeBase) + sizeof(PtrType) + str_size);
    return tmp;

  } else if (type->kind == TP_Fn) {
    usize str_size = sizeof(char) * type->fn_type.name.size;
    Type* tmp = parser_alloc(sizeof(TypeBase) + sizeof(FnType) + str_size);
    strncpy((str)tmp, (str)type, sizeof(TypeBase) + sizeof(FnType) + str_size);
    return tmp;

  } else if (type->kind == TP_Int) {
    usize str_size = sizeof(char) * type->name.size;
    Type* tmp = parser_alloc(sizeof(TypeBase) + str_size);
    strncpy(
      (str)tmp, (str)type, sizeof(TypeBase) + sizeof(VarCharArr) + str_size
    );
    return tmp;
  }
  error("Invalid type found");
}

Type* make_ptr_type(Type* base) {
  Type* type =
    parser_alloc(sizeof(TypeBase) + sizeof(PtrType) + sizeof(char) * 32);
  *type = (Type){
    .kind = TP_Ptr,
    .ptr_type =
      (PtrType){
        .base = base,
        .name.size = 32,
      },
  };
  return type;
}

Type* make_decl_type(TypeKind kind) {
  Type* type =
    parser_alloc(sizeof(TypeBase) + sizeof(VarCharArr) + sizeof(char) * 32);
  *type = (Type){
    .kind = kind,
    .name.size = 32,
  };
  return type;
}

Type* make_fn_type(Type* args, Type* ret) {
  Type* type =
    parser_alloc(sizeof(TypeBase) + sizeof(FnType) + sizeof(char) * 32);
  *type = (Type){
    .kind = TP_Fn,
    .fn_type =
      (FnType){
        .args = args,
        .ret = ret,
        .name.size = 32,
      },
  };
  return type;
}
