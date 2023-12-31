#include <lexer/errors.h>
#include <parser/ctors.h>
#include <parser/mod.h>
#include <string.h>

void free_none(any) {
}
DEFINE_VECTOR(NodeRef, parser_alloc, free_none);

// Node* make_add(Node* lhs, Node* rhs, Token* token) {
//   add_type(lhs);
//   add_type(rhs);

//   if (is_numeric(lhs->type) && is_numeric(rhs->type)) {
//     return make_oper(OP_Add, lhs, rhs);

//   } else if (lhs->type->kind == TP_Ptr && rhs->type->kind == TP_Ptr) {
//     error_tok(token, "invalid operands");

//   } else if (lhs->type->kind != TP_Ptr && rhs->type->kind == TP_Ptr) {
//     Node* tmp = lhs;
//     lhs = rhs;
//     rhs = tmp;
//   }
//   rhs = make_oper(OP_Mul, rhs, make_number(token->pos));
//   return make_oper(OP_Add, lhs, rhs);
// }

// Node* make_sub(Node* lhs, Node* rhs, Token* token) {
//   add_type(lhs);
//   add_type(rhs);

//   if (is_numeric(lhs->type) && is_numeric(rhs->type)) {
//     return make_oper(OP_Sub, lhs, rhs);

//   } else if (lhs->type->kind == TP_Ptr && is_numeric(rhs->type)) {
//     rhs = make_oper(OP_Mul, rhs, make_number(token->pos));
//     add_type(rhs);
//     Node* node = make_oper(OP_Sub, lhs, rhs);
//     node->type = lhs->type;
//     return node;

//   } else if (lhs->type->kind == TP_Ptr && lhs->type->kind == TP_Ptr) {
//     Node* node = make_oper(OP_Sub, lhs, rhs);
//     node->type = make_decl_type(TP_Int);
//     return make_oper(OP_Div, node, make_number(token->pos));
//   }
//   error_tok(token, "Invalid operands");
// }

Node* make_oper(OperKind oper, Node* lhs, Node* rhs) {
  Node* node = parser_alloc(sizeof(NodeBase) + sizeof(OperNode));
  *node = (Node){
    .base.kind = ND_Operation,
    .operation =
      (OperNode){
        .kind = oper,
        .lhs = lhs,
        .rhs = rhs,
      },
  };
  return node;
}

Node* make_unary(NodeKind kind, Node* value) {
  Node* node = parser_alloc(sizeof(NodeBase) + sizeof(Node*));
  *node = (Node){
    .base.kind = kind,
    .unary = value,
  };
  return node;
}

Node* make_basic_value(Node* type, StrView view) {
  usize size = view.sen - view.itr;
  Node* node = parser_alloc(
    sizeof(NodeBase) + sizeof(ValueNode) + sizeof(char) * (size + 1)
  );
  *node = (Node){
    .base.kind = ND_Value,
    .value =
      (ValueNode){
        .kind = type->value.kind,
        .type = type,
        .basic.size = size,
      },
  };
  strncpy(node->value.basic.array, view.itr, size);
  node->value.basic.array[size] = 0;
  return node;
}

Node* make_basic_type(TypeKind kind) {
  Node* node = parser_alloc(sizeof(NodeBase) + sizeof(ValueNode));
  *node = (Node){
    .base.kind = ND_Type,
    .value =
      (ValueNode){
        .is_type = true,
        .kind = kind,
      },
  };
  return node;
}

Node* make_pointer_value(Node* type, Node* value) {
  Node* node = parser_alloc(sizeof(NodeBase) + sizeof(ValueNode));
  *node = (Node){
    .base.kind = ND_Value,
    .value =
      (ValueNode){
        .kind = TP_Ptr,
        .type = type,
        .base = value,
      },
  };
  return node;
}

Node* make_pointer_type(Node* type) {
  Node* node = parser_alloc(sizeof(NodeBase) + sizeof(ValueNode));
  *node = (Node){
    .base.kind = ND_Type,
    .value =
      (ValueNode){
        .is_type = true,
        .kind = TP_Ptr,
        .type = type,
      },
  };
  return node;
}

Node* make_declaration(Node* type, StrView view, Node* value) {
  if (current_locals == nullptr) {
    current_locals = NodeRef_vector_make(8);
  }
  usize size = view.sen - view.itr;
  Node* node = parser_alloc(
    sizeof(NodeBase) + sizeof(DeclNode) + sizeof(char) * (size + 1)
  );
  *node = (Node){
    .base.kind = ND_Decl,
    .declaration =
      (DeclNode){
        .type = type,
        .value = value,
        .name.size = size,
      },
  };
  strncpy(node->declaration.name.array, view.itr, size);
  node->declaration.name.array[size] = 0;
  NodeRef_vector_push(&current_locals, node);
  return node;
}

Node* make_arg_var(Node* type, StrView view) {
  if (current_locals == nullptr) {
    current_locals = NodeRef_vector_make(8);
  }
  usize size = view.sen - view.itr;
  Node* node = parser_alloc(
    sizeof(NodeBase) + sizeof(DeclNode) + sizeof(char) * (size + 1)
  );
  *node = (Node){
    .base.kind = ND_ArgVar,
    .declaration =
      (DeclNode){
        .type = type,
        .name.size = size,
      },
  };
  strncpy(node->declaration.name.array, view.itr, size);
  node->declaration.name.array[size] = 0;
  NodeRef_vector_push(&current_locals, node);
  return node;
}

Node* make_function(Node* type, StrView view, Node* body, Node* args) {
  usize size = view.sen - view.itr;
  Node* node =
    parser_alloc(sizeof(NodeBase) + sizeof(FnNode) + sizeof(char) * (size + 1));
  *node = (Node){
    .base.kind = ND_Function,
    .function =
      (FnNode){
        .body = body,
        .args = args,
        .ret_type = type,
        .locals = current_locals,
        .name.size = size,
      },
  };
  strncpy(node->function.name.array, view.itr, size);
  node->function.name.array[size] = 0;
  return node;
}

Node* make_if_node(Node* cond, Node* then, Node* elseb) {
  Node* node = parser_alloc(sizeof(NodeBase) + sizeof(IfNode));
  *node = (Node){
    .base.kind = ND_If,
    .if_node =
      (IfNode){
        .cond = cond,
        .then = then,
        .elseb = elseb,
      },
  };
  return node;
}

Node* make_while_node(Node* cond, Node* then) {
  Node* node = parser_alloc(sizeof(NodeBase) + sizeof(WhileNode));
  *node = (Node){
    .base.kind = ND_While,
    .while_node =
      (WhileNode){
        .cond = cond,
        .then = then,
      },
  };
  return node;
}

Node* make_call_node(StrView view, Node* args) {
  usize size = view.sen - view.itr;
  Node* node = parser_alloc(
    sizeof(NodeBase) + sizeof(CallNode) + sizeof(char) * (size + 1)
  );
  *node = (Node){
    .base.kind = ND_Call,
    .call_node =
      (CallNode){
        .args = args,
        .name.size = size,
      },
  };
  strncpy(node->call_node.name.array, view.itr, size);
  node->call_node.name.array[size] = 0;
  return node;
}
