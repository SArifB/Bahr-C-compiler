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
  Type* tmp;
  if (type->kind == TP_Ptr) {
    tmp = parser_alloc(sizeof(TypeBase) + sizeof(PtrType));
    strncpy((str)tmp, (str)type, sizeof(TypeBase) + sizeof(PtrType));
  } else if (type->kind == TP_Fn) {
    tmp = parser_alloc(sizeof(TypeBase) + sizeof(FnType));
    strncpy((str)tmp, (str)type, sizeof(TypeBase) + sizeof(FnType));
  } else if (type->kind == TP_Int) {
    tmp = parser_alloc(sizeof(TypeBase));
    strncpy((str)tmp, (str)type, sizeof(TypeBase));
  } else {
    exit(1);
  }
  return tmp;
}

Type* make_ptr_type(Type* base) {
  Type* type = parser_alloc(sizeof(TypeBase) + sizeof(PtrType));
  *type = (Type){
    .kind = TP_Ptr,
    .ptr_type.base = base,
  };
  return type;
}

Type* make_decl_type(TypeKind kind) {
  // usize size = view.sen - view.itr;
  // Type* type = parser_alloc(sizeof(TypeKind) + sizeof(char) * (size + 1));
  Type* type = parser_alloc(sizeof(TypeBase));
  *type = (Type){
    .kind = kind,
    // .decl_name.size = size,
  };
  // strncpy(type->decl_name.array, view.itr, size);
  // type->decl_name.array[size] = 0;
  return type;
}

Type* make_fn_type(Type* args, Type* ret) {
  Type* type = parser_alloc(sizeof(TypeBase) + sizeof(FnType));
  *type = (Type){
    .kind = TP_Fn,
    .fn_type =
      (FnType){
        .args = args,
        .ret = ret,
      },
  };
  return type;
}
