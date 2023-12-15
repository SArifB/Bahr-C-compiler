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
  } else if (token->info == PK_Add || token->info == PK_Sub) {
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

static Node* make_unary(Node* value) {
  Node* node = parser_alloc(sizeof(Node));
  *node = (Node){
    .kind = ND_Neg,
    .unaval = value,
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
  strncpy(node->value.str_lit.arr, view.itr, size);
  return node;
}

// expr = equality
Node* expr(Token** rest, Token* token) {
  return equality(rest, token);
}

// equality = relational ("==" relational | "!=" relational)*
Node* equality(Token** rest, Token* token) {
  Node* node = relational(&token, token);

  while (true) {
    if (token->info == PK_Eq) {
      node = make_oper(OP_Eq, node, relational(&token, token + 1));
    } else if (token->info == PK_NEq) {
      node = make_oper(OP_NEq, node, relational(&token, token + 1));
    } else {
      *rest = token;
      return node;
    }
  }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node* relational(Token** rest, Token* token) {
  Node* node = add(&token, token);

  while (true) {
    if (token->info == PK_Lt) {
      node = make_oper(OP_Lt, node, add(&token, token + 1));
    } else if (token->info == PK_Lte) {
      node = make_oper(OP_Lte, node, add(&token, token + 1));
    } else if (token->info == PK_Gt) {
      node = make_oper(OP_Gt, node, add(&token, token + 1));
    } else if (token->info == PK_Gte) {
      node = make_oper(OP_Gte, node, add(&token, token + 1));
    } else {
      *rest = token;
      return node;
    }
  }
}

// add = mul ("+" mul | "-" mul)*
Node* add(Token** rest, Token* token) {
  Node* node = mul(&token, token);

  while (true) {
    if (token->info == PK_Add) {
      node = make_oper(OP_Add, node, mul(&token, token + 1));
    } else if (token->info == PK_Sub) {
      node = make_oper(OP_Sub, node, mul(&token, token + 1));
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
  if (token->info == PK_Add) {
    return unary(rest, token + 1);
  } else if (token->info == PK_Sub) {
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
  static i32 indent = 0;
  // eprintf("%*s", indent, "| ");
  for (i32 i = 0; i < indent / 2; ++i) {
    eputc('|');
    eputc(' ');
  }
  indent += 1;

  if (node == nullptr) {
    // return;

  } else if (node->kind == ND_Neg) {
    eputs("ND_Neg: ");
    indent += 1;
    print_ast_tree(node->unaval);
    indent -= 1;

  } else if (node->kind == ND_Oper) {
    // clang-format off
    switch (node->binop.kind) {
      case OP_Add:  eputs("ND_Oper: OP_Add"); break;
      case OP_Sub:  eputs("ND_Oper: OP_Sub"); break;
      case OP_Mul:  eputs("ND_Oper: OP_Mul"); break;
      case OP_Div:  eputs("ND_Oper: OP_Div"); break;
      case OP_Eq:   eputs("ND_Oper: OP_Eq");  break;
      case OP_NEq:  eputs("ND_Oper: OP_Not"); break;
      case OP_Lt:   eputs("ND_Oper: OP_Lt");  break;
      case OP_Lte:  eputs("ND_Oper: OP_Lte"); break;
      case OP_Gte:  eputs("ND_Oper: OP_Gte"); break;
      case OP_Gt:   eputs("ND_Oper: OP_Gt");  break;
    }
    // clang-format on
    indent += 1;
    print_ast_tree(node->binop.lhs);
    print_ast_tree(node->binop.rhs);
    indent -= 1;

  } else if (node->kind == ND_Val) {
    if (node->value.kind == TP_Int) {
      eprintf("ND_Val: TP_Int = %li\n", node->value.i_num);
    }
  }
  indent -= 1;
}

void free_ast_tree() {
  arena_free(&PARSER_ARENA);
}
