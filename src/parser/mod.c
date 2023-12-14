#include <arena/mod.h>
#include <lexer/mod.h>
#include <parser/mod.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility/mod.h>

static Arena PARSER_ARENA = {};
#define parser_alloc(size) arena_alloc(&PARSER_ARENA, size)

unused static i32 get_tok_precedence(Token* token) {
  if (token->info == PK_Mul || token->info == PK_Div) {
    return 5;
  } else if (token->info == PK_Plus || token->info == PK_Minus) {
    return 4;
  } else if (token->info == PK_Eq) {
    return 3;
  } else if (token->info == PK_Lte || token->info == PK_Gte) {
    return 2;
  } else if (token->info == PK_Gt || token->info == PK_Lt) {
    return 1;
  }
  return 0;
}

static __attribute((noreturn)) void print_expr_err(cstr itr) {
  eputs("Error: Invalid expression at: ");
  while (*itr != '\n') {
    eputc(*itr++);
  }
  exit(-2);
}

static Node* make_oper(OperKind oper, Node* lhs, Node* rhs) {
  Node* node = parser_alloc(sizeof(Node));
  *node = (Node){
    .kind = ND_Oper,
    .binop =
      (OperNode){
        .kind = oper,
        .lhs = lhs,
        .rhs = rhs,
      },
  };
  return node;
}

static Node* make_unary(Node* lhs) {
  Node* node = parser_alloc(sizeof(Node));
  *node = (Node){
    .kind = ND_Neg,
    .binop =
      (OperNode){
        .kind = OP_Neg,
        .lhs = lhs,
      },
  };
  return node;
}

static Node* make_number(StrView view) {
  Node* node = parser_alloc(sizeof(Node));
  *node = (Node){
    .kind = ND_Val,
    .value =
      (ValueNode){
        .kind = TP_Int,
        .i_num = strtol(view.itr, nullptr, 10),
      },
  };
  return node;
}

unused static Node* make_float(StrView view) {
  Node* node = parser_alloc(sizeof(Node));
  *node = (Node){
    .kind = ND_Val,
    .value =
      (ValueNode){
        .kind = TP_Flt,
        .f_num = strtod(view.itr, nullptr),
      },
  };
  return node;
}

unused static Node* make_string(StrView view) {
  usize size = view.sen - view.itr;
  Node* node = parser_alloc(sizeof(Node) + sizeof(char) * (size + 1));
  *node = (Node){
    .kind = ND_Val,
    .value =
      (ValueNode){
        .kind = TP_Str,
        .str_lit =
          (VarCharArr){
            .size = size,
          },
      },
  };
  strncpy(node->value.str_lit.arr, view.itr, size - 1);
  return node;
}

// expr = mul ("+" mul | "-" mul)*
Node* expr(Token** rest, Token* token) {
  Node* node = mul(&token, token);

  while (true) {
    if (token->info == PK_Plus) {
      node = make_oper(OP_Plus, node, mul(&token, token + 1));
    } else if (token->info == PK_Minus) {
      node = make_oper(OP_Minus, node, mul(&token, token + 1));
    } else {
      *rest = token;
      return node;
    }
  }
}

// mul = unary ("*" unary | "/" unary)*
Node* mul(Token** rest, Token* token) {
  Node* node = unary(&token, token);

  while (true) {
    if (token->info == PK_Mul) {
      node = make_oper(OP_Mul, node, unary(&token, token + 1));
    } else if (token->info == PK_Div) {
      node = make_oper(OP_Div, node, unary(&token, token + 1));
    } else {
      *rest = token;
      return node;
    }
  }
}

// unary = ("+" | "-") unary
//       | primary
Node* unary(Token** rest, Token* token) {
  if (token->info == PK_Plus) {
    return unary(rest, token + 1);
  } else if (token->info == PK_Minus) {
    return make_unary(unary(rest, token + 1));
  }
  return primary(rest, token);
}

// primary = "(" expr ")" | num
Node* primary(Token** rest, Token* token) {
  if (token->info == PK_LeftParen) {
    Node* node = expr(&token, token + 1);
    if (token->info == PK_RightParen) {
      *rest = token + 1;
      return node;
    }
  } else if (token->kind == TK_NumLiteral) {
    Node* node = make_number(token->pos);
    *rest = token + 1;
    return node;
  }
  print_expr_err(token->pos.itr);
}

void print_ast_tree(Node* node) {
  if (node == nullptr) {
    return;

  } else if (node->kind == ND_Oper) {
    print_ast_tree(node->binop.lhs);
    if (node->binop.kind == OP_Plus) {
      eputs("ND_Oper: OP_Plus");
    } else if (node->binop.kind == OP_Minus) {
      eputs("ND_Oper: OP_Minus");
    }
    print_ast_tree(node->binop.rhs);

  } else if (node->kind == ND_Neg) {
    eputs("ND_Neg: OP_Neg");
    print_ast_tree(node->binop.lhs);

  } else if (node->kind == ND_Val) {
    if (node->value.kind == TP_Int) {
      eprintf("ND_Val: TP_Int = %li\n", node->value.i_num);
    }
  }
}

void free_ast_tree() {
  arena_free(&PARSER_ARENA);
}
