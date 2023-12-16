#include <arena/mod.h>
#include <lexer/mod.h>
#include <parser/mod.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility/mod.h>

static i32 get_tok_precedence(Token* token);
static __attribute((noreturn)) void print_expr_err(cstr itr);
static Node* make_oper(OperKind oper, Node* lhs, Node* rhs);
static Node* make_unary(UnaryKind kind, Node* value);
static Node* make_number(StrView view);
static Node* make_float(StrView view);
static Node* make_string(StrView view);
static Node* make_variable(StrView view);
static Token* skip(Token* token, AddInfo info);

static Node* stmt(Token** rest, Token* token);
static Node* expr_stmt(Token** rest, Token* token);
static Node* assign(Token** rest, Token* tok);
static Node* expr(Token** rest, Token* token);
static Node* equality(Token** rest, Token* token);
static Node* relational(Token** rest, Token* token);
static Node* add(Token** rest, Token* token);
static Node* mul(Token** rest, Token* token);
static Node* unary(Token** rest, Token* token);
static Node* primary(Token** rest, Token* token);

// stmt = "return" expr ";"
//      | expr-stmt
static Node* stmt(Token** rest, Token* token) {
  if (token->info == KW_Return) {
    Node* node = make_unary(UN_Return, expr(&token, token + 1));
    *rest = skip(token, PK_SemiCol);
    return node;
  }

  return expr_stmt(rest, token);
}

// expr-stmt = expr ";"
static Node* expr_stmt(Token** rest, Token* token) {
  Node* node = make_unary(UN_ExprStmt, expr(&token, token));
  *rest = skip(token, PK_SemiCol);
  return node;
}

// expr = assign
static Node* expr(Token** rest, Token* token) {
  return assign(rest, token);
}

// assign = equality ("=" assign)?
static Node* assign(Token** rest, Token* token) {
  Node* node = equality(&token, token);
  if (token->info == PK_Assign) {
    node = make_oper(OP_Asg, node, assign(&token, token + 1));
  }
  *rest = token;
  return node;
}

// equality = relational ("==" relational | "!=" relational)*
static Node* equality(Token** rest, Token* token) {
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
static Node* relational(Token** rest, Token* token) {
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
static Node* add(Token** rest, Token* token) {
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
static Node* mul(Token** rest, Token* token) {
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
static Node* unary(Token** rest, Token* token) {
  if (token->info == PK_Add) {
    return unary(rest, token + 1);
  } else if (token->info == PK_Sub) {
    return make_unary(UN_Negation, unary(rest, token + 1));
  }
  return primary(rest, token);
}

// primary = "(" expr ")" | ident | num
static Node* primary(Token** rest, Token* token) {
  if (token->info == PK_LeftParen) {
    Node* node = expr(&token, token + 1);
    *rest = skip(token, PK_RightParen);
    return node;

  } else if (token->kind == TK_Ident) {
    Node* node = make_variable(token->pos);
    *rest = token + 1;
    return node;

  } else if (token->kind == TK_NumLiteral) {
    Node* node = make_number(token->pos);
    *rest = token + 1;
    return node;
  }
  print_expr_err(token->pos.itr);
}

// program = stmt*
Node* parse_lexer(TokenVector* tokens) {
  Token* tok_cur = tokens->buffer;
  Node* node_cur = nullptr;
  while (tok_cur->kind != TK_EOF) {
    node_cur = stmt(&tok_cur, tok_cur);
  }
  return node_cur;
}

static Arena PARSER_ARENA = {};
#define parser_alloc(size) arena_alloc(&PARSER_ARENA, size)

void free_ast() {
  arena_free(&PARSER_ARENA);
}

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

static void print_expr_err(cstr itr) {
  eputs("Error: Invalid expression at: ");
  while (*itr != '\n') {
    eputc(*itr++);
  }
  exit(-2);
}

static Node* make_oper(OperKind oper, Node* lhs, Node* rhs) {
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

static Node* make_unary(UnaryKind kind, Node* value) {
  Node* node = parser_alloc(sizeof(Node));
  *node = (Node){
    .kind = ND_Unary,
    .unary =
      (UnaryNode){
        .kind = kind,
        .next = value,
      },
  };
  return node;
}

static Node* make_number(StrView view) {
  Node* node = parser_alloc(sizeof(Node));
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

unused static Node* make_float(StrView view) {
  Node* node = parser_alloc(sizeof(Node));
  *node = (Node){
    .kind = ND_Value,
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
    .kind = ND_Value,
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
  node->value.str_lit.arr[size] = 0;
  return node;
}

static Node* make_variable(StrView view) {
  usize size = view.sen - view.itr;
  Node* node = parser_alloc(sizeof(Node) + sizeof(char) * (size + 1));
  *node = (Node){
    .kind = ND_Variable,
    .variable =
      (VarNode){
        .name =
          (VarCharArr){
            .size = size,
          },
      },
  };
  strncpy(node->variable.name.arr, view.itr, size);
  node->variable.name.arr[size] = 0;
  return node;
}

static Token* skip(Token* token, AddInfo info) {
  if (token->info != info) {
    print_expr_err(token->pos.itr);
  }
  return token + 1;
}

void print_ast(Node* node) {
  static i32 indent = 0;
  for (i32 i = 0; i < indent; ++i) {
    eputc('|');
    eputc(' ');
  }
  indent += 1;

  if (node == nullptr) {
    // return;

  } else if (node->kind == ND_Variable) {
    eprintf("ND_Vari: %s\n", node->variable.name.arr);

  } else if (node->kind == ND_Unary) {
    // clang-format off
    switch (node->unary.kind) {
      case UN_Negation: eputs("ND_Unary: UN_Negation"); break;
      case UN_ExprStmt: eputs("ND_Unary: UN_ExprStmt"); break;
      case UN_Return:   eputs("ND_Unary: UN_Return");   break;
    }
    // clang-format on
    print_ast(node->unary.next);

  } else if (node->kind == ND_Operation) {
    // clang-format off
    switch (node->operation.kind) {
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
      case OP_Asg:  eputs("ND_Oper: OP_Asg"); break;
    }
    // clang-format on
    print_ast(node->operation.lhs);
    print_ast(node->operation.rhs);

  } else if (node->kind == ND_Value) {
    if (node->value.kind == TP_Int) {
      eprintf("ND_Val: TP_Int = %li\n", node->value.i_num);
    }
  }
  indent -= 1;
}
