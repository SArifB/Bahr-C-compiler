#include <lexer/errors.h>
#include <parser/ctors.h>
#include <parser/mod.h>
#include <parser/types.h>
#include <string.h>

Node* make_add(Node* lhs, Node* rhs, Token* token) {
  add_type(lhs);
  add_type(rhs);

  if (is_numeric(lhs->type) && is_numeric(rhs->type)) {
    return make_oper(OP_Add, lhs, rhs);

  } else if (lhs->type->kind == TP_Ptr && rhs->type->kind == TP_Ptr) {
    error_tok(token, "invalid operands");

  } else if (lhs->type->kind != TP_Ptr && rhs->type->kind == TP_Ptr) {
    Node* tmp = lhs;
    lhs = rhs;
    rhs = tmp;
  }
  rhs = make_oper(OP_Mul, rhs, make_number(token->pos));
  return make_oper(OP_Add, lhs, rhs);
}

Node* make_sub(Node* lhs, Node* rhs, Token* token) {
  add_type(lhs);
  add_type(rhs);

  if (is_numeric(lhs->type) && is_numeric(rhs->type)) {
    return make_oper(OP_Sub, lhs, rhs);

  } else if (lhs->type->kind == TP_Ptr && is_numeric(rhs->type)) {
    rhs = make_oper(OP_Mul, rhs, make_number(token->pos));
    add_type(rhs);
    Node* node = make_oper(OP_Sub, lhs, rhs);
    node->type = lhs->type;
    return node;

  } else if (lhs->type->kind == TP_Ptr && lhs->type->kind == TP_Ptr) {
    Node* node = make_oper(OP_Sub, lhs, rhs);
    node->type = make_decl_type(TP_Int);
    return make_oper(OP_Div, node, make_number(token->pos));
  }
  error_tok(token, "Invalid operands");
}

Node* make_oper(OperKind oper, Node* lhs, Node* rhs) {
  Node* node = parser_alloc(sizeof(NodeBase) + sizeof(OperNode));
  *node = (Node){
    .kind = ND_Operation,
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
    .kind = kind,
    .unary = value,
  };
  return node;
}

Node* make_number(StrView view) {
  Node* node = parser_alloc(sizeof(NodeBase) + sizeof(ValueNode));
  *node = (Node){
    .kind = ND_Value,
    .value =
      (ValueNode){
        .kind = TP_Int,
        .i_num = strtol(view.itr, nullptr, 10),
      },
  };
  return node;
}

Node* make_variable(Object* obj) {
  Node* node = parser_alloc(sizeof(NodeBase) + sizeof(Object*));
  *node = (Node){
    .kind = ND_Variable,
    .variable = obj,
  };
  return node;
}

Node* make_if_node(Node* cond, Node* then, Node* elseb) {
  Node* node = parser_alloc(sizeof(NodeBase) + sizeof(IfNode));
  *node = (Node){
    .kind = ND_If,
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
    .kind = ND_While,
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
    .kind = ND_Call,
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

Object* make_object(StrView view, Type* type) {
  usize size = view.sen - view.itr;
  Object* obj = parser_alloc(sizeof(Object) + sizeof(char) * (size + 1));
  *obj = (Object){
    .next = current_locals,
    .type = type,
    .name.size = size,
  };
  strncpy(obj->name.array, view.itr, size);
  obj->name.array[size] = 0;
  current_locals = obj;
  return obj;
}

Function* make_function(StrView view, Node* body, Object* args) {
  usize size = view.sen - view.itr;
  Function* func = parser_alloc(sizeof(Function) + sizeof(char) * (size + 1));
  *func = (Function){
    .body = body,
    .args = args,
    .locals = current_locals,
    .name.size = size,
  };
  strncpy(func->name.array, view.itr, size);
  func->name.array[size] = 0;
  return func;
}
