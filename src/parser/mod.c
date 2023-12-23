#include <arena/mod.h>
#include <lexer/errors.h>
#include <lexer/mod.h>
#include <parser/ctors.h>
#include <parser/mod.h>
#include <parser/types.h>
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

Object* current_locals = nullptr;

static Object* find_var(Token* token) {
  for (Object* obj = current_locals; obj; obj = obj->next) {
    if (strncmp(token->pos.itr, obj->name.array, token->pos.sen - token->pos.itr) == 0) {
      return obj;
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
static Type* declspec(Token** rest, Token* token) {
  if (strncmp(token->pos.itr, "int", 3) == 0) {
    *rest = token + 1;
    return nullptr;
    // Type* type = make_decl_type(TP_Int);
    // strncpy(type->name.array, "int", 3);
    // type->name.array[3] = 0;
    // return type;
  }
  error_tok(token, "Invalid expression");
}

static Type* type_suffix(Token** rest, Token* token, Type* type);
static Type* declarator(Token** rest, Token* token, Type* type);
static Node* declaration(Token** rest, Token* token);
static Node* stmt(Token** rest, Token* token);
static Node* compound_stmt(Token** rest, Token* token);
static Node* expr_stmt(Token** rest, Token* token);
static Node* assign(Token** rest, Token* token);
static Node* expr(Token** rest, Token* token);
static Node* equality(Token** rest, Token* token);
static Node* relational(Token** rest, Token* token);
static Node* add(Token** rest, Token* token);
static Node* mul(Token** rest, Token* token);
static Node* unary(Token** rest, Token* token);
static Node* primary(Token** rest, Token* token);
static Function* function(Token** rest, Token* token);

// type-suffix = ("(" func-params)?
static Type* type_suffix(Token** rest, Token* token, Type* type) {
  if (token->info != PK_LeftParen) {
    token = token + 1;
    Type head = {};
    Type* cur = &head;
    while (token->info != PK_RightParen) {
      if (cur != &head)
        token = expect_info(token, PK_Comma);
      Type* basety = declspec(&token, token);
      Type* ty = declarator(&token, token, basety);
      cur->next = copy_type(ty);
      cur = cur->next;
    }
    type = make_fn_type(head.next, type);
    *rest = token + 1;
    return type;
  }
  *rest = token;
  return type;
}

// declarator = "*"* ident type-suffix
static Type* declarator(Token** rest, Token* token, Type* type) {
  while (consume(&token, token, PK_Mul)) {
    type = make_ptr_type(type);
  }
  if (token->kind != TK_Ident) {
    error_tok(token, "Expected a variable name");
  }
  type = type_suffix(rest, token + 1, type);
  // type->name = token;
  return type;
}

// declaration = declspec (declarator ("=" expr)?
//               ("," declarator ("=" expr)?)*)? ";"
unused static Node* og_declaration(Token** rest, Token* token) {
  Type* base_ty = declspec(&token, token);

  Node handle = {};
  Node* node_cursor = &handle;
  while (token->info != PK_SemiCol) {
    if (node_cursor != &handle) {
      token = expect_info(token, PK_Comma);
    }
    Type* type = declarator(&token, token, base_ty);
    Object* var = make_object(view_from(&type->name));

    if (token->info != PK_Assign) {
      continue;
    }
    Node* lhs = make_variable(var);
    Node* rhs = assign(&token, token + 1);
    Node* node = make_oper(OP_Asg, lhs, rhs);
    node_cursor = node_cursor->next = make_unary(ND_ExprStmt, node);
  }

  Node* node = make_unary(ND_Block, handle.next);
  *rest = token + 1;
  return node;
}

static Node* declaration(Token** rest, Token* token) {
  StrView name = token->pos;
  token = expect_ident(token);
  token = expect_info(token, PK_Colon);
  unused Type* decl_tp = declspec(rest, token);
  return make_variable(make_object(name));
}

// stmt = "return" expr
//      | "if" expr stmt ("else" stmt)?
//      | "while" expr stmt
//      | "let" expr stmt
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
    unused Node* variable = declaration(&token, token + 1);
    Node* value = stmt(rest, expect_info(token, PK_Assign));
    return value;

  } else if (token->info == PK_LeftBracket) {
    return compound_stmt(rest, token + 1);
  }
  return expr_stmt(rest, token);
}

// compound-stmt = stmt* "}"
static Node* compound_stmt(Token** rest, Token* token) {
  Node handle = {};
  Node* node_cursor = &handle;
  while (token->info != PK_RightBracket) {
    // if (strncmp(token->pos.itr, "int", 3) == 0) {
    //   node_cursor->next = declaration(&token, token);
    //   node_cursor = node_cursor->next;
    // } else {
    node_cursor->next = stmt(&token, token);
    node_cursor = node_cursor->next;
    // }
    // add_type(node_cursor);
  }
  Node* node = make_unary(ND_Block, handle.next);
  *rest = token + 1;
  return node;
}

// expr-stmt = expr?
static Node* expr_stmt(Token** rest, Token* token) {
  if (token->info == PK_SemiCol) {
    error_tok(token, "Unnecessary semicolon");
  }
  Node* node = make_unary(ND_ExprStmt, expr(&token, token));
  *rest = expect_eol(token - 1);
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
  Token* start = token;
  token = token + 2;

  Node handle = {};
  Node* node_cursor = &handle;
  while (token->info != PK_RightParen) {
    if (node_cursor != &handle) {
      token = expect_info(token, PK_Comma);
    }
    node_cursor->next = assign(&token, token);
    node_cursor = node_cursor->next;
  }
  // Node* node = make_call_node(start->pos, handle.next);
  Node* node = make_call_node(start->pos);
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
    Object* obj = find_var(token);
    if (obj == nullptr) {
      // obj = make_object(token->pos, nullptr);
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

unused static void create_param_lvars(Type* arg) {
  if (arg) {
    create_param_lvars(arg->next);
    // make_object(view_from(&arg->name), arg);
    make_object(view_from(&arg->name));
  }
}

// program = stmt*
static Function* function(Token** rest, Token* token) {
  StrView name = token->pos;
  token = expect_ident(token);
  token = expect_info(token, PK_LeftParen);

  Node handle = {};
  Node* node_cursor = &handle;
  while (token->info != PK_RightParen) {
    if (node_cursor != &handle) {
      token = expect_info(token, PK_Comma);
    }
    node_cursor->next = declaration(&token, token);
    node_cursor = node_cursor->next;
  }
  unused Type* ret_tp = declspec(&token, expect_info(token + 1, PK_Colon));
  unused Node* body = compound_stmt(&token, expect_info(token, PK_LeftBracket));
  unused Function* func = make_function(name, body);

  *rest = token + 1;
  return func;
}

// program = fn*
Function* parse_lexer(TokenVector* tokens) {
  Token* token = tokens->buffer;

  Function handle = {};
  Function* node_cursor = &handle;
  while (token->kind != TK_EOF) {
    node_cursor->next = function(&token, expect_info(token, KW_Fn));
    node_cursor = node_cursor->next;
  }
  return handle.next;
}
