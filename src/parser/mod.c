#include <arena/mod.h>
#include <lexer/errors.h>
#include <lexer/mod.h>
#include <parser/mod.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility/mod.h>

static Arena PARSER_ARENA = {};
#define parser_alloc(size) arena_alloc(&PARSER_ARENA, size)

void free_ast() {
  arena_free(&PARSER_ARENA);
}

static Object* current_locals = nullptr;

static Object* find_var(Token* token) {
  for (Object* obj = current_locals; obj; obj = obj->next) {
    if (strncmp(token->pos.itr, obj->name.arr, token->pos.sen - token->pos.itr) == 0) {
      return obj;
    }
  }
  return nullptr;
}

static Node* make_oper(OperKind oper, Node* lhs, Node* rhs) {
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

static Node* make_negation(Node* value) {
  Node* node = parser_alloc(sizeof(NodeBase) + sizeof(Node*));
  *node = (Node){
    .kind = ND_Negation,
    .unary = value,
  };
  return node;
}

static Node* make_expr_stmt(Node* value) {
  Node* node = parser_alloc(sizeof(NodeBase) + sizeof(Node*));
  *node = (Node){
    .kind = ND_ExprStmt,
    .expr_stmt = value,
  };
  return node;
}

static Node* make_return_node(Node* value) {
  Node* node = parser_alloc(sizeof(NodeBase) + sizeof(Node*));
  *node = (Node){
    .kind = ND_Return,
    .return_node = value,
  };
  return node;
}

static Node* make_number(StrView view) {
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

unused static Node* make_float(StrView view) {
  Node* node = parser_alloc(sizeof(NodeBase) + sizeof(ValueNode));
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
  Node* node = parser_alloc(
    sizeof(NodeBase) + sizeof(TypeKind) + sizeof(char) * (size + 1)
  );
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

static Node* make_variable(Object* obj) {
  Node* node = parser_alloc(sizeof(NodeBase) + sizeof(Object*));
  *node = (Node){
    .kind = ND_Variable,
    .variable = obj,
  };
  return node;
}

static Node* make_block(Node* block) {
  Node* node = parser_alloc(sizeof(NodeBase) + sizeof(Node*));
  *node = (Node){
    .kind = ND_Block,
    .block = block,
  };
  return node;
}

static Node* make_if_node(Node* cond, Node* then, Node* elseb) {
  Node* node = parser_alloc(sizeof(NodeBase) + sizeof(IfNode));
  *node = (Node){
    .kind = ND_If,
    .ifblock =
      (IfNode){
        .cond = cond,
        .then = then,
        .elseb = elseb,
      },
  };
  return node;
}

static Node* make_while_node(Node* cond, Node* then) {
  Node* node = parser_alloc(sizeof(NodeBase) + sizeof(WhileNode));
  *node = (Node){
    .kind = ND_While,
    .whileblock =
      (WhileNode){
        .cond = cond,
        .then = then,
      },
  };
  return node;
}

static Node* make_call_node(StrView view, Node* args) {
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

static Object* make_object(StrView view) {
  usize size = view.sen - view.itr;
  Object* obj = parser_alloc(sizeof(Object) + sizeof(char) * (size + 1));
  *obj = (Object){
    .next = current_locals,
    .name =
      (VarCharArr){
        .size = size,
      },
  };
  strncpy(obj->name.arr, view.itr, size);
  obj->name.arr[size] = 0;
  current_locals = obj;
  return obj;
}

static Function* make_function(Node* node) {
  Function* func = parser_alloc(sizeof(Function));
  *func = (Function){
    .body = node,
    .locals = current_locals,
  };
  return func;
}

static Token* expect(Token* token, AddInfo info) {
  if (token->info != info) {
    error_tok(token - 1, "Invalid expression");
  }
  return token + 1;
}

static Node* stmt(Token** rest, Token* token);
static Node* compound_stmt(Token** rest, Token* tok);
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
//      | "if" expr stmt ("else" stmt)?
//      | "while" expr stmt
//      | "{" compound-stmt
//      | expr-stmt
static Node* stmt(Token** rest, Token* token) {
  if (token->info == KW_Return) {
    Node* node = make_unary(UN_Return, expr(&token, token + 1));
    *rest = expect(token, PK_SemiCol);
    return node;

  } else if (token->info == KW_If) {
    Node* cond = expr(&token, token + 1);
    Node* then = stmt(&token, token);
    Node* elseb = nullptr;
    if (token->info == KW_Else) {
      elseb = stmt(&token, token + 1);
    }
    Node* node = make_if_node(cond, then, elseb);
    *rest = token;
    return node;

  } else if (token->info == KW_While) {
    Node* cond = expr(&token, token + 1);
    Node* then = stmt(rest, token);
    Node* node = make_while_node(cond, then);
    return node;

  } else if (token->info == PK_LeftBracket) {
    return compound_stmt(rest, token + 1);
  }
  return expr_stmt(rest, token);
}

// compound-stmt = stmt* "}"
static Node* compound_stmt(Token** rest, Token* token) {
  Node handle = {};
  Node* node_cur = &handle;
  while (token->info != PK_RightBracket) {
    node_cur->next = stmt(&token, token);
    node_cur = node_cur->next;
  }
  Node* node = make_block(handle.next);
  *rest = token + 1;
  return node;
}

// expr-stmt = expr? ";"
static Node* expr_stmt(Token** rest, Token* token) {
  if (token->info == PK_SemiCol) {
    *rest = token + 1;
    return make_block(nullptr);
  }
  Node* node = make_unary(UN_ExprStmt, expr(&token, token));
  *rest = expect(token, PK_SemiCol);
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

// funcall = ident "(" (assign ("," assign)*)? ")"
static Node* funcall(Token** rest, Token* token) {
  Token* start = token;
  token = token + 2;

  Node handle = {};
  Node* node_cursor = &handle;
  while (token->info != PK_RightParen) {
    if (node_cursor != &handle) {
      token = expect(token, PK_Comma);
    }
    node_cursor->next = assign(&token, token);
    node_cursor = node_cursor->next;
  }
  Node* node = make_call_node(start->pos, handle.next);
  *rest = expect(token, PK_RightParen);
  return node;
}

// primary = "(" expr ")" | ident func-args? | num
static Node* primary(Token** rest, Token* token) {
  if (token->info == PK_LeftParen) {
    Node* node = expr(&token, token + 1);
    *rest = expect(token, PK_RightParen);
    return node;

  } else if (token->kind == TK_Ident) {
    if ((token + 1)->info == PK_LeftParen) {
      return funcall(rest, token);
    }
    Object* obj = find_var(token);
    if (obj == nullptr) {
      obj = make_object(token->pos);
    }
    *rest = token + 1;
    return make_variable(obj);

  } else if (token->kind == TK_NumLiteral) {
    Node* node = make_number(token->pos);
    *rest = token + 1;
    return node;
  }
  error_tok(token, "Expected an expression");
}

// program = stmt*
Function* parse_lexer(TokenVector* tokens) {
  Token* token = tokens->buffer;
  token = expect(token, PK_LeftBracket);
  return make_function(compound_stmt(&token, token));
}

static i32 indent = 0;
static void print_indent() {
  for (i32 i = 0; i < indent; ++i) {
    eputc('|');
    eputc(' ');
  }
  indent += 1;
}

static void print_branch(Node* node) {
  print_indent();

  if (node->kind == ND_None) {
    eputs("ND_None");

  } else if (node->kind == ND_Operation) {
    switch (node->operation.kind) { // clang-format off
      case OP_Add:  eputs("ND_Operation: OP_Add");  break;
      case OP_Sub:  eputs("ND_Operation: OP_Sub");  break;
      case OP_Mul:  eputs("ND_Operation: OP_Mul");  break;
      case OP_Div:  eputs("ND_Operation: OP_Div");  break;
      case OP_Eq:   eputs("ND_Operation: OP_Eq");   break;
      case OP_NEq:  eputs("ND_Operation: OP_Not");  break;
      case OP_Lt:   eputs("ND_Operation: OP_Lt");   break;
      case OP_Lte:  eputs("ND_Operation: OP_Lte");  break;
      case OP_Gte:  eputs("ND_Operation: OP_Gte");  break;
      case OP_Gt:   eputs("ND_Operation: OP_Gt");   break;
      case OP_Asg:  eputs("ND_Operation: OP_Asg");  break;
    } // clang-format on
    print_branch(node->operation.lhs);
    print_branch(node->operation.rhs);

  } else if (node->kind == ND_Unary) {
    switch (node->unary.kind) { // clang-format off
      case UN_Negation: eputs("ND_Unary: UN_Negation"); break;
      case UN_ExprStmt: eputs("ND_Unary: UN_ExprStmt"); break;
      case UN_Return:   eputs("ND_Unary: UN_Return");   break;
    } // clang-format on
    print_branch(node->unary.next);

  } else if (node->kind == ND_Variable) {
    eprintf("ND_Variable: %s\n", node->variable->name.arr);

  } else if (node->kind == ND_Value) {
    if (node->value.kind == TP_Int) {
      eprintf("ND_Value: TP_Int = %li\n", node->value.i_num);
    } else {
      eputs("unimplemented");
    }
  } else if (node->kind == ND_Block) {
    eputs("ND_Block:");
    for (Node* branch = node->block; branch != nullptr; branch = branch->next) {
      print_branch(branch);
    }
  } else if (node->kind == ND_If) {
    eputs("ND_If: Cond");
    print_branch(node->ifblock.cond);

    indent -= 1;
    print_indent();
    eputs("ND_If: Then");
    print_branch(node->ifblock.then);

    if (node->ifblock.elseb) {
      indent -= 1;
      print_indent();
      eputs("ND_If: Else");
      print_branch(node->ifblock.elseb);
    }
  } else if (node->kind == ND_While) {
    eputs("ND_While: Cond");
    print_branch(node->whileblock.cond);

    indent -= 1;
    print_indent();
    eputs("ND_While: Then");
  } else if (node->kind == ND_Call) {
    eprintf("ND_Call: %s\n", node->call_node.name.array);
    for (Node* branch = node->call_node.args; branch != nullptr;
         branch = branch->next) {
      print_branch(branch);
    }
  }
  indent -= 1;
}

void print_ast(Function* prog) {
  print_branch(prog->body);
}
