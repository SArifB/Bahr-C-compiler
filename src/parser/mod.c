#include <arena/mod.h>
#include <lexer/errors.h>
#include <lexer/mod.h>
#include <parser/ctors.h>
#include <parser/mod.h>
#include <string.h>
#include <utility/mod.h>

Arena PARSER_ARENA = {};

void free_ast() {
  arena_free(&PARSER_ARENA);
}

any parser_alloc(usize size) {
  return arena_alloc(&PARSER_ARENA, size);
}

#define view_from(var_arr)                                                     \
  ((StrView){ (var_arr)->array, (var_arr)->array + (var_arr)->size })

NodeRefVector* current_locals = nullptr;

static Node* find_var(Token* token) {
  Node** itr = current_locals->buffer;
  Node** sen = current_locals->buffer + current_locals->length;
  usize size = token->pos.sen - token->pos.itr;
  for (; itr != sen; ++itr) {
    if (strncmp(token->pos.itr, (*itr)->variable.name.array, size) == 0) {
      return *itr;
    }
  }
  return nullptr;
}

unused static bool consume(Token** rest, Token* token, AddInfo info) {
  if (token->info != info) {
    *rest = token;
    return false;
  }
  *rest = token + 1;
  return true;
}

static Token* expect_info(Token* token, AddInfo info) {
  if (token->info != info) {
    error_tok(token, "Invalid expression");
  }
  return token + 1;
}

static Token* expect_eol(Token* token) {
  if (token->is_eol != true) {
    error_tok(token + 1, "Expected end of the line");
  }
  return token + 1;
}

static Token* expect_ident(Token* token) {
  if (token->kind != TK_Ident) {
    error_tok(token, "Expected an identifier");
  }
  return token + 1;
}

// declspec = "int"
static TypeKind declspec(Token** rest, Token* token) {
  if (strncmp(token->pos.itr, "int", 3) == 0) {
    *rest = token + 1;
    return TP_Int;
  }
  error_tok(token, "Invalid expression");
}

static Node* declaration(Token** rest, Token* token);
static Node* stmt(Token** rest, Token* token);
static Node* compound_stmt(Token** rest, Token* token);
static Node* assign(Token** rest, Token* token);
static Node* expr(Token** rest, Token* token);
static Node* equality(Token** rest, Token* token);
static Node* relational(Token** rest, Token* token);
static Node* add(Token** rest, Token* token);
static Node* mul(Token** rest, Token* token);
static Node* unary(Token** rest, Token* token);
static Node* primary(Token** rest, Token* token);
static Node* function(Token** rest, Token* token);

static Node* declaration(Token** rest, Token* token) {
  StrView name = token->pos;
  token = expect_ident(token);
  token = expect_info(token, PK_Colon);
  Node* var = make_variable(name);
  TypeKind decl_tp = declspec(rest, token);
  var->variable.type = decl_tp;
  return var;
}

// stmt = "return" expr
//      | "if" expr stmt ("else" stmt)?
//      | "while" expr stmt
//      | "let" decl "=" expr
//      | "{" compound-stmt
//      | expr-stmt
static Node* stmt(Token** rest, Token* token) {
  if (token->info == KW_Return) {
    Node* node = make_unary(ND_Return, expr(&token, token + 1));
    *rest = expect_eol(token - 1);
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

  } else if (token->info == KW_Let) {
    Node* var = declaration(&token, token + 1);
    Node* value = expr(rest, expect_info(token, PK_Assign));
    var->variable.value = value;
    return make_oper(OP_Decl, var, value);

  } else if (token->info == PK_LeftBracket) {
    return compound_stmt(rest, token + 1);
  }
  return expr(rest, token);
}

// compound-stmt = stmt* "}"
static Node* compound_stmt(Token** rest, Token* token) {
  Node handle = {};
  Node* node_cursor = &handle;
  while (token->info != PK_RightBracket) {
    token = expect_eol(token - 1);
    node_cursor->next = stmt(&token, token);
    node_cursor = node_cursor->next;
  }
  Node* node = make_unary(ND_Block, handle.next);
  *rest = token + 1;
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
      // node = make_add(node, mul(&token, token + 1), token);
      node = make_oper(OP_Add, node, mul(&token, token + 1));
    } else if (token->info == PK_Sub) {
      // node = make_sub(node, mul(&token, token + 1), token);
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

// unary = ("+" | "-" | "*" | "&") unary
//       | primary
static Node* unary(Token** rest, Token* token) {
  if (token->info == PK_Add) {
    return unary(rest, token + 1);
  } else if (token->info == PK_Sub) {
    return make_unary(ND_Negation, unary(rest, token + 1));
  } else if (token->info == PK_Ampersand) {
    return make_unary(ND_Addr, unary(rest, token + 1));
  } else if (token->info == PK_Mul) {
    return make_unary(ND_Deref, unary(rest, token + 1));
  }
  return primary(rest, token);
}

// funcall = ident "(" (assign ("," assign)*)? ")"
static Node* fn_call(Token** rest, Token* token) {
  StrView name = token->pos;
  token = token + 2;

  Node handle = {};
  Node* node_cursor = &handle;
  while (token->info != PK_RightParen) {
    if (node_cursor != &handle) {
      token = expect_info(token, PK_Comma);
    }
    if (token->info == PK_RightParen) {
      break;
    }
    node_cursor->next = assign(&token, token);
    node_cursor = node_cursor->next;
  }
  Node* node = make_call_node(name, handle.next);
  *rest = expect_info(token, PK_RightParen);
  return node;
}

// primary = "(" expr ")" | ident func-args? | num
static Node* primary(Token** rest, Token* token) {
  if (token->info == PK_LeftParen) {
    Node* node = expr(&token, token + 1);
    *rest = expect_info(token, PK_RightParen);
    return node;

  } else if (token->kind == TK_Ident) {
    if ((token + 1)->info == PK_LeftParen) {
      return fn_call(rest, token);
    }
    Node* var = find_var(token);
    if (var == nullptr) {
      error_tok(token, "Variable not found in scope");
    }
    *rest = token + 1;
    return var;

  } else if (token->kind == TK_NumLiteral) {
    Node* node = make_basic_value(TP_Int, token->pos);
    *rest = token + 1;
    return node;
  }
  error_tok(token, "Expected an expression");
}

// program = stmt*
static Node* function(Token** rest, Token* token) {
  StrView name = token->pos;
  token = expect_ident(token);
  token = expect_info(token, PK_LeftParen);
  current_locals = nullptr;

  Node handle = {};
  Node* cursor = &handle;
  while (token->info != PK_RightParen) {
    if (cursor != &handle) {
      token = expect_info(token, PK_Comma);
    }
    if (token->info == PK_RightParen) {
      break;
    }
    cursor->next = declaration(&token, token);
    cursor = cursor->next;
  };
  TypeKind ret_tp = declspec(&token, expect_info(token + 1, PK_Colon));
  Node* body = compound_stmt(rest, expect_info(token, PK_LeftBracket));
  Node* func = make_function(name, body, handle.next);
  func->function.ret_type = ret_tp;
  return func;
}

// program = function*
Node* parse_lexer(TokenVector* tokens) {
  Token* token = tokens->buffer;

  Node handle = {};
  Node* cursor = &handle;
  while (token->kind != TK_EOF) {
    cursor->next = function(&token, expect_info(token, KW_Fn));
    cursor = cursor->next;
  }
  return handle.next;
}
