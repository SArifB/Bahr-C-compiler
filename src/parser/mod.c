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

NodeRefVector* current_locals = nullptr;

static Node* find_variable(Token* token) {
  Node** itr = current_locals->buffer;
  Node** sen = current_locals->buffer + current_locals->length;
  usize size = token->pos.sen - token->pos.itr;
  for (; itr != sen; ++itr) {
    if (
      strncmp(token->pos.itr, (*itr)->declaration.name.array, size) == 0
      && (*itr)->declaration.name.array[size] =='\0') {
      return make_unary(ND_Variable, (*itr));
    }
  }
  return nullptr;
}

static bool consume(Token** rest, Token* token, AddInfo info) {
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
  if (strncmp(token->pos.itr, "i32", strlen("i32")) == 0) {
    *rest = token + 1;
    return TP_Int;
  }
  error_tok(token, "Invalid expression");
}

static Node* parse_list(
  Token** rest, Token* token, AddInfo breaker,
  fn(Node*(Token**, Token*)) callable
) {
  Node handle = {};
  Node* cursor = &handle;
  while (token->info != breaker) {
    if (cursor != &handle) {
      token = expect_info(token, PK_Comma);
    }
    if (token->info == breaker) {
      break;
    }
    cursor->next = callable(&token, token);
    cursor = cursor->next;
  };
  *rest = token;
  return handle.next;
}

static Node* function(Token** rest, Token* token);
static Node* extern_function(Token** rest, Token* token);
static Node* argument(Token** rest, Token* token);
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

// program = functions*
Node* parse_lexer(TokenVector* tokens) {
  Token* token = tokens->buffer;

  Node handle = {};
  Node* cursor = &handle;
  while (token->kind != TK_EOF) {
    if (consume(&token, token, KW_Ext)) {
      cursor->next = extern_function(&token, expect_info(token, KW_Fn));
      cursor = cursor->next;

    } else {
      cursor->next = function(&token, expect_info(token, KW_Fn));
      cursor = cursor->next;
    }
  }
  return handle.next;
}

// function = indent "(" args? ")" ":" ret_type "{" body "}"
static Node* function(Token** rest, Token* token) {
  StrView name = token->pos;
  token = expect_ident(token);
  token = expect_info(token, PK_LeftParen);
  current_locals = nullptr;

  Node* args = parse_list(&token, token, PK_RightParen, argument);
  TypeKind ret_type = declspec(&token, expect_info(token + 1, PK_Colon));
  Node* body = compound_stmt(rest, expect_info(token, PK_LeftBracket));
  return make_function(ret_type, name, body, args);
}

// extern_function = indent "(" args? ")" ":" ret_type
static Node* extern_function(Token** rest, Token* token) {
  StrView name = token->pos;
  token = expect_ident(token);
  token = expect_info(token, PK_LeftParen);

  Node* args = parse_list(&token, token, PK_RightParen, argument);
  TypeKind ret_type = declspec(rest, expect_info(token + 1, PK_Colon));
  return make_function(ret_type, name, nullptr, args);
}

// argument = indent ":" type
static Node* argument(Token** rest, Token* token) {
  StrView name = token->pos;
  token = expect_ident(token);
  TypeKind decl_type = declspec(rest, expect_info(token, PK_Colon));
  return make_arg_var(decl_type, name);
}

// declaration = indent ":" type "=" expr
static Node* declaration(Token** rest, Token* token) {
  StrView name = token->pos;
  token = expect_ident(token);
  TypeKind decl_type = declspec(&token, expect_info(token, PK_Colon));
  Node* value = expr(rest, expect_info(token, PK_Assign));
  Node* var = make_declaration(decl_type, name, value);
  return var;
}

// stmt = "return" expr
//      | "if" expr stmt ("else" stmt)?
//      | "while" expr stmt
//      | "let" decl ":" type "=" expr
//      | "{" compound-stmt
//      | expr
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
    Node* decl = declaration(rest, token + 1);
    return decl;

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
    Node* var = find_variable(token);
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
