#include <arena/mod.h>
#include <hashmap/mod.h>
#include <parser/ctors.h>
#include <parser/lexer.h>
#include <parser/mod.h>
#include <string.h>
#include <utility/mod.h>

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

Node* make_oper(Arena* arena, OperKind oper, Node* lhs, Node* rhs) {
  Node* node = arena_alloc(arena, sizeof(Node));
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

Node* make_unary(Arena* arena, NodeKind kind, Node* value) {
  Node* node = arena_alloc(arena, sizeof(Node));
  *node = (Node){
    .kind = kind,
    .unary = value,
  };
  return node;
}

static StrNode* alloc_string(Arena* arena, StrView view) {
  usize size = sizeof(usize) + sizeof(char) * (view.length + 1);
  StrNode* string = arena_alloc(arena, size);
  memcpy(string->array, view.pointer, view.length);
  string->array[view.length] = 0;
  string->capacity = view.length;
  return string;
}

Node* make_basic_value(Arena* arena, Node* type, StrView view) {
  Node* node = arena_alloc(arena, sizeof(Node));
  *node = (Node){
    .kind = ND_Value,
    .value =
      (ValueNode){
        .type = type,
        .basic = alloc_string(arena, view),
      },
  };
  return node;
}

static StrNode* alloc_str_lit(Arena* arena, StrView view) {
  usize size = sizeof(usize) + sizeof(char) * (view.length + 1);
  StrNode* string = arena_alloc(arena, size);
  rcstr src = view.pointer;
  rstr itr = string->array;
  size = view.length;

  for (; src != view.pointer + view.length; ++itr, ++src) {
    if (*src == '\\' && src[1] == 'n') {
      *itr = '\n';
      src += 1;
      size -= 1;
    } else {
      *itr = *src;
    }
  }
  *itr = 0;
  string->capacity = size;
  return string;
}

Node* make_str_value(Arena* arena, StrView view) {
  Node* node = arena_alloc(arena, sizeof(Node));
  *node = (Node){
    .kind = ND_Value,
    .value =
      (ValueNode){
        .type = make_basic_type(arena, TP_Str),
        .basic = alloc_str_lit(arena, view),
      },
  };
  return node;
}

Node* make_numeric_value(Arena* arena, Node* type, usize number) {
  Node* node = arena_alloc(arena, sizeof(Node));
  *node = (Node){
    .kind = ND_Value,
    .value =
      (ValueNode){
        .type = type,
        .number = number,
      },
  };
  return node;
}

Node* make_pointer_value(Arena* arena, Node* type, Node* value) {
  Node* node = arena_alloc(arena, sizeof(Node));
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

Node* make_basic_type(Arena* arena, TypeKind kind) {
  Node* node = arena_alloc(arena, sizeof(Node));
  *node = (Node){
    .kind = ND_Type,
    .type =
      (TypeNode){
        .kind = kind,
      },
  };
  return node;
}

Node* make_numeric_type(Arena* arena, TypeKind kind, usize width) {
  Node* node = arena_alloc(arena, sizeof(Node));
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

Node* make_pointer_type(Arena* arena, Node* type) {
  Node* node = arena_alloc(arena, sizeof(Node));
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

Node* make_array_type(Arena* arena, Node* type, usize size) {
  Node* node = arena_alloc(arena, sizeof(Node));
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

Node* make_declaration(Context cx, Node* type, StrView view, Node* value) {
  Node* node = arena_alloc(cx.arena, sizeof(Node));
  *node = (Node){
    .kind = ND_Decl,
    .declaration =
      (DeclNode){
        .type = type,
        .value = value,
        .name = alloc_string(cx.arena, view),
      },
  };
  *hashmap_get(cx.scopes->buffer + cx.scopes->length - 1, view) = node;
  return node;
}

Node* make_arg_var(Context cx, Node* type, StrView view) {
  Node* node = arena_alloc(cx.arena, sizeof(Node));
  *node = (Node){
    .kind = ND_ArgVar,
    .declaration =
      (DeclNode){
        .type = type,
        .name = alloc_string(cx.arena, view),
      },
  };
  *hashmap_get(cx.scopes->buffer + cx.scopes->length - 1, view) = node;
  return node;
}

Node* make_function(
  Arena* arena, Node* type, StrView view, Node* body, Node* args,
  Linkage linkage
) {
  Node* node = arena_alloc(arena, sizeof(Node));
  *node = (Node){
    .kind = ND_Function,
    .function =
      (FnNode){
        .linkage = linkage,
        .body = body,
        .args = args,
        .ret_type = type,
        .name = alloc_string(arena, view),
      },
  };
  return node;
}

Node* make_if_node(Arena* arena, Node* cond, Node* then, Node* elseb) {
  Node* node = arena_alloc(arena, sizeof(Node));
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

Node* make_while_node(Arena* arena, Node* cond, Node* then) {
  Node* node = arena_alloc(arena, sizeof(Node));
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

Node* make_call_node(Arena* arena, StrView view, Node* args) {
  Node* node = arena_alloc(arena, sizeof(Node));
  *node = (Node){
    .kind = ND_Call,
    .call_node =
      (CallNode){
        .args = args,
        .name = alloc_string(arena, view),
      },
  };
  return node;
}
