#include <arena/mod.h>
#include <hashmap/mod.h>
#include <parser/ctors.h>
#include <parser/lexer.h>
#include <parser/mod.h>
#include <string.h>
#include <utility/mod.h>

static Arena PARSER_POOL = {};

static void* parser_alloc(usize size) {
  return arena_alloc(&PARSER_POOL, size);
}

void parser_dealloc() {
  arena_free(&PARSER_POOL);
}

// static bool is_integer(Node* node) {
//   return node->kind == ND_Value &&
//          (node->value.kind == TP_SInt || node->value.kind == TP_UInt);
// }

// unused static bool is_pointer(Node* node) {
//   return (node->kind == ND_Value && node->value.kind == TP_Ptr);
// }

// unused Node* make_add(Node* lhs, Node* rhs, Token* token) {
//   if (is_pointer(lhs) == false && is_pointer(rhs) == false) {
//     if (is_integer(lhs) == true && is_integer(rhs) == true) {
//       return make_oper(OP_Add, lhs, rhs);
//     }
//   } else if (is_pointer(lhs) == true && is_pointer(rhs) == true) {
//     error_tok(token, "invalid operands, cant add two pointers");

//   } else if (is_pointer(lhs) == false && is_pointer(rhs) == true) {
//     return make_oper(OP_PtrAdd, rhs, lhs);
//   }
//   return make_oper(OP_PtrAdd, lhs, rhs);
// }

// unused Node* make_sub(Node* lhs, Node* rhs, Token* token) {
//   if (is_pointer(lhs) == false && is_pointer(rhs) == false) {
//     if (is_integer(lhs) == true && is_integer(rhs) == true) {
//       return make_oper(OP_Sub, lhs, rhs);
//     }
//   } else if (is_pointer(lhs) == true && is_pointer(rhs) == true) {
//     return make_oper(OP_PtrPtrSub, lhs, rhs);

//   } else if (is_pointer(lhs) == false && is_pointer(rhs) == true) {
//     error_tok(token, "Invalid operands, cant sub pointer from integer");
//   }
//   return make_oper(OP_PtrSub, lhs, rhs);
// }

Node* make_oper(OperKind oper, Node* lhs, Node* rhs) {
  Node* node = parser_alloc(sizeof(Node));
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
  Node* node = parser_alloc(sizeof(Node));
  *node = (Node){
    .kind = kind,
    .unary = value,
  };
  return node;
}

static StrArr* alloc_string(StrView view) {
  usize size = sizeof(usize) + sizeof(char) * (view.length + 1);
  StrArr* string = parser_alloc(size);
  memcpy(string->array, view.ptr, view.length);
  string->array[view.length] = 0;
  string->length = view.length;
  return string;
}

Node* make_basic_value(Node* type, StrView view) {
  Node* node = parser_alloc(sizeof(Node));
  *node = (Node){
    .kind = ND_Value,
    .value =
      (ValueNode){
        .type = type,
        .basic = alloc_string(view),
      },
  };
  return node;
}

static StrArr* alloc_str_lit(StrView view) {
  usize size = sizeof(usize) + sizeof(char) * (view.length + 1);
  StrArr* string = parser_alloc(size);
  cstr src = view.ptr;
  str itr = string->array;
  size = view.length;

  for (; src != view.ptr + view.length; ++itr, ++src) {
    if (*src == '\\' && src[1] == 'n') {
      *itr = '\n';
      src += 1;
      size -= 1;
    } else {
      *itr = *src;
    }
  }
  *itr = 0;
  string->length = size;
  return string;
}

Node* make_str_value(StrView view) {
  Node* node = parser_alloc(sizeof(Node));
  *node = (Node){
    .kind = ND_Value,
    .value =
      (ValueNode){
        .type = make_basic_type(TP_Str),
        .basic = alloc_str_lit(view),
      },
  };
  return node;
}

Node* make_numeric_value(Node* type, u64 num) {
  Node* node = parser_alloc(sizeof(Node));
  *node = (Node){
    .kind = ND_Value,
    .value =
      (ValueNode){
        .type = type,
        .number = num,
      },
  };
  return node;
}

Node* make_pointer_value(Node* type, Node* value) {
  Node* node = parser_alloc(sizeof(Node));
  *node = (Node){
    .kind = ND_Value,
    .value =
      (ValueNode){
        .type = type,
        .base = value,
      },
  };
  return node;
}

Node* make_basic_type(TypeKind kind) {
  Node* node = parser_alloc(sizeof(Node));
  *node = (Node){
    .kind = ND_Type,
    .type =
      (TypeNode){
        .kind = kind,
      },
  };
  return node;
}

Node* make_numeric_type(TypeKind kind, i32 width) {
  Node* node = parser_alloc(sizeof(Node));
  *node = (Node){
    .kind = ND_Type,
    .type =
      (TypeNode){
        .kind = kind,
        .bit_width = width,
      },
  };
  return node;
}

Node* make_pointer_type(Node* type) {
  Node* node = parser_alloc(sizeof(Node));
  *node = (Node){
    .kind = ND_Type,
    .type =
      (TypeNode){
        .kind = TP_Ptr,
        .base = type,
      },
  };
  return node;
}

Node* make_array_type(Node* type, usize size) {
  Node* node = parser_alloc(sizeof(Node));
  *node = (Node){
    .kind = ND_Type,
    .type =
      (TypeNode){
        .kind = TP_Arr,
        .array.base = type,
        .array.size = size,
      },
  };
  return node;
}

Node* make_declaration(Node* type, StrView view, Node* value, Scopes* scopes) {
  Node* node = parser_alloc(sizeof(Node));
  *node = (Node){
    .kind = ND_Decl,
    .declaration =
      (DeclNode){
        .type = type,
        .value = value,
        .name = alloc_string(view),
      },
  };
  *hashmap_get(scopes->buffer + scopes->length - 1, view) = node;
  return node;
}

Node* make_arg_var(Node* type, StrView view, Scopes* scopes) {
  Node* node = parser_alloc(sizeof(Node));
  *node = (Node){
    .kind = ND_ArgVar,
    .declaration =
      (DeclNode){
        .type = type,
        .name = alloc_string(view),
      },
  };
  *hashmap_get(scopes->buffer + scopes->length - 1, view) = node;
  return node;
}

Node* make_function(Node* type, StrView view, Node* body, Node* args) {
  Node* node = parser_alloc(sizeof(Node));
  *node = (Node){
    .kind = ND_Function,
    .function =
      (FnNode){
        .body = body,
        .args = args,
        .ret_type = type,
        .name = alloc_string(view),
      },
  };
  return node;
}

Node* make_if_node(Node* cond, Node* then, Node* elseb) {
  Node* node = parser_alloc(sizeof(Node));
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
  Node* node = parser_alloc(sizeof(Node));
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
  Node* node = parser_alloc(sizeof(Node));
  *node = (Node){
    .kind = ND_Call,
    .call_node =
      (CallNode){
        .args = args,
        .name = alloc_string(view),
      },
  };
  return node;
}
