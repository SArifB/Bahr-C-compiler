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
    .binop = (OperNode){ .kind = oper, .lhs = lhs, .rhs = rhs },
  };
  return node;
}

static Node* make_number(i64 value) {
  Node* node = parser_alloc(sizeof(Node));
  *node = (Node){
    .kind = ND_Val,
    .value =
      (ValueNode){
        .kind = TP_Int,
        .i_num = value,
      },
  };
  return node;
}

unused static Node* make_float(f64 value) {
  Node* node = parser_alloc(sizeof(Node));
  *node = (Node){
    .kind = ND_Val,
    .value =
      (ValueNode){
        .kind = TP_Flt,
        .f_num = value,
      },
  };
  return node;
}

unused static Node* make_string(StrView view) {
  usize size = view.sen - view.itr + 1;
  Node* node = parser_alloc(sizeof(Node) + sizeof(char) * size);
  *node = (Node){
    .kind = ND_Val,
    .value =
      (ValueNode){
        .kind = TP_Str,
        .str_lit = (VarCharArr){ .size = size },
      },
  };
  strncpy(node->value.str_lit.arr, view.itr, size);
  return node;
}

// expr = mul ("+" mul | "-" mul)*
Node* expr(Token** rest, Token* tok) {
  Node* node = mul(&tok, tok);

  for (;;) {
    if (tok->info == PK_Plus) {
      node = make_oper(OP_Plus, node, mul(&tok, tok + 1));
      continue;
    }

    if (tok->info == PK_Minus) {
      node = make_oper(OP_Minus, node, mul(&tok, tok + 1));
      continue;
    }

    *rest = tok;
    return node;
  }
}

// mul = primary ("*" primary | "/" primary)*
Node* mul(Token** rest, Token* tok) {
  Node* node = primary(&tok, tok);

  for (;;) {
    if (tok->info == PK_Mul) {
      node = make_oper(OP_Mul, node, primary(&tok, tok + 1));
      continue;
    }
    if (tok->info == PK_Div) {
      node = make_oper(OP_Div, node, primary(&tok, tok + 1));
      continue;
    }
    *rest = tok;
    return node;
  }
}

// primary = "(" expr ")" | num
Node* primary(Token** rest, Token* tok) {
  if (tok->info == PK_LeftParen) {
    Node* node = expr(&tok, tok + 1);
    if (tok->info == PK_RightParen) {
      *rest = tok + 1;
      return node;
    }
  }
  if (tok->kind == TK_NumLiteral) {
    Node* node = make_number(strtol(tok->pos.itr, nullptr, 10));
    *rest = tok + 1;
    return node;
  }
  print_expr_err(tok->pos.itr);
}

void print_ast_tree(Node* node) {
  if (node == nullptr) {
    return;

  } else if (node->kind == ND_Oper) {
    if (node->binop.kind == OP_Plus) {
      eputs("OP_Plus: +");
    }
    print_ast_tree(node->binop.lhs);
    print_ast_tree(node->binop.rhs);

  } else if (node->kind == ND_Val) {
    if (node->value.kind == TP_Int) {
      eprintf("ND_Val: TP_Int = %li\n", node->value.i_num);
    }
  }
}

void free_ast_tree() {
  arena_free(&PARSER_ARENA);
}
