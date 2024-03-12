#include <arena/mod.h>
#include <hashmap/mod.h>
#include <parser/ctors.h>
#include <parser/lexer.h>
#include <parser/mod.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility/mod.h>
#include <utility/vec.h>

static bool print_verbose = false;

void enable_verbosity() {
  print_verbose = true;
}

static Node* parse_lexer(TokenVector* tokens);

Node* parse_string(const StrView view) {
  TokenVector* tokens = lex_string(view);
  if (print_verbose) {
    lexer_print(tokens);
    eputs("\n-----------------------------------------------");
  }
  Node* prog = parse_lexer(tokens);
  if (print_verbose) {
    print_ast(prog);
    eputs("\n-----------------------------------------------");
  }
  free(tokens);
  return prog;
}

DEFINE_VEC_FNS(Scope, malloc, free)

static Node* find_variable(Token* token, Scopes* scopes) {
  Scope* rev_itr = scopes->buffer + scopes->length - 1;
  Scope* rev_sen = scopes->buffer - 1;

  for (; rev_itr != rev_sen; --rev_itr) {
    Node* var = *hashmap_find(rev_itr, token->pos);
    if (var != nullptr) {
      return make_unary(ND_Variable, var);
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

static Node* parse_list(
  Token** rest, Token* token, Scopes* scopes, AddInfo breaker,
  fn(Node*(Token**, Token*, Scopes*)) callable
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
    cursor->next = callable(&token, token, scopes);
    cursor = cursor->next;
  };
  *rest = token;
  return handle.next;
}

static Node* parse_type(Token** rest, Token* token, Scopes*);
static Node* extern_function(Token** rest, Token* token, Scopes*);
static Node* function(Token** rest, Token* token, Scopes*);
static Node* argument(Token** rest, Token* token, Scopes*);
static Node* declaration(Token** rest, Token* token, Scopes*);
static Node* stmt(Token** rest, Token* token, Scopes*);
static Node* compound_stmt(Token** rest, Token* token, Scopes*);
static Node* assign(Token** rest, Token* token, Scopes*);
static Node* expr(Token** rest, Token* token, Scopes*);
static Node* equality(Token** rest, Token* token, Scopes*);
static Node* relational(Token** rest, Token* token, Scopes*);
static Node* add(Token** rest, Token* token, Scopes*);
static Node* mul(Token** rest, Token* token, Scopes*);
static Node* unary(Token** rest, Token* token, Scopes*);
static Node* primary(Token** rest, Token* token, Scopes*);

// parse_type = "[" num "]" | "*" | "i"num | "f"num
static Node* parse_type(Token** rest, Token* token, Scopes* scopes) {
  if (token->kind == TK_Punct) {
    if (consume(&token, token, PK_Mul)) {
      return make_pointer_type(parse_type(rest, token, scopes));

    } else if (consume(&token, token, PK_LeftBracket)) {
      Node* size = expr(&token, token, scopes);
      Node* type =
        parse_type(rest, expect_info(token, PK_RightBracket), scopes);
      if (size->kind == ND_Value) {
        return make_array_type(type, atoi(size->value.basic->array));
      } else {
        error_tok(token, "Size value not found");
      }
    }
  } else if (token->kind == TK_Ident) {
    if (token->info == AD_SIntType) {
      i32 width = atoi(token->pos.ptr + 1);
      if (width > 128) {
        error_tok(token, "Bit width too wide");
      }
      Node* type = make_numeric_type(TP_SInt, width);
      *rest = token + 1;
      return type;

    } else if (token->info == AD_UIntType) {
      i32 width = atoi(token->pos.ptr + 1);
      if (width > 128) {
        error_tok(token, "Bit width too wide");
      }
      Node* type = make_numeric_type(TP_UInt, width);
      *rest = token + 1;
      return type;

    } else if (token->info == AD_BF16Type) {
      Node* type = make_numeric_type(TP_Flt, 15);
      *rest = token + 1;
      return type;

    } else if (token->info == AD_F16Type) {
      Node* type = make_numeric_type(TP_Flt, 16);
      *rest = token + 1;
      return type;

    } else if (token->info == AD_F32Type) {
      Node* type = make_numeric_type(TP_Flt, 32);
      *rest = token + 1;
      return type;

    } else if (token->info == AD_F64Type) {
      Node* type = make_numeric_type(TP_Flt, 64);
      *rest = token + 1;
      return type;

    } else if (token->info == AD_F128Type) {
      Node* type = make_numeric_type(TP_Flt, 128);
      *rest = token + 1;
      return type;
    }
  }
  error_tok(token, "Invalid expression");
}

// program = functions*
static Node* parse_lexer(TokenVector* tokens) {
  Token* token = tokens->buffer;
  Token* sentinel = tokens->buffer + tokens->length;
  Scopes* scopes = Scope_vector_make(8);
  Scope_vector_push(&scopes, hashmap_make(32));

  Node handle = {};
  Node* cursor = &handle;
  while (token != sentinel) {
    if (consume(&token, token, KW_Ext)) {
      cursor->next = extern_function(&token, expect_info(token, KW_Fn), scopes);
      cursor = cursor->next;

    } else {
      cursor->next = function(&token, expect_info(token, KW_Fn), scopes);
      cursor = cursor->next;
    }
  }
  return handle.next;
}

// extern_function = indent "(" args? ")" ":" ret_type
static Node* extern_function(Token** rest, Token* token, Scopes* scopes) {
  StrView name = token->pos;
  token = expect_ident(token);
  token = expect_info(token, PK_LeftParen);

  Node* args = parse_list(&token, token, scopes, PK_RightParen, argument);
  Node* type = parse_type(rest, expect_info(token + 1, PK_Colon), scopes);
  return make_function(type, name, nullptr, args);
}

// function = indent "(" args? ")" ":" ret_type "{" body "}"
static Node* function(Token** rest, Token* token, Scopes* scopes) {
  StrView name = token->pos;
  token = expect_ident(token);
  token = expect_info(token, PK_LeftParen);
  Scope_vector_push(&scopes, hashmap_make(8));

  Node* args = parse_list(&token, token, scopes, PK_RightParen, argument);
  Node* type = parse_type(&token, expect_info(token + 1, PK_Colon), scopes);
  Node* body = compound_stmt(rest, expect_info(token, PK_LeftBrace), scopes);

  hashmap_free(scopes->buffer[scopes->length - 1]);
  Scope_vector_pop(scopes);
  return make_function(type, name, body, args);
}

// argument = indent ":" type
static Node* argument(Token** rest, Token* token, Scopes* scopes) {
  StrView name = token->pos;
  token = expect_ident(token);
  Node* type = parse_type(rest, expect_info(token, PK_Colon), scopes);
  return make_arg_var(type, name, scopes);
}

// declaration = indent ":" type "=" expr
static Node* declaration(Token** rest, Token* token, Scopes* scopes) {
  StrView name = token->pos;
  token = expect_ident(token);
  Node* type = parse_type(&token, expect_info(token, PK_Colon), scopes);
  Node* value = expr(rest, expect_info(token, PK_Assign), scopes);
  return make_declaration(type, name, value, scopes);
}

// stmt = "return" expr
//      | "if" expr stmt ("else" stmt)?
//      | "while" expr stmt
//      | "let" decl ":" type "=" expr
//      | "{" compound-stmt
//      | expr
static Node* stmt(Token** rest, Token* token, Scopes* scopes) {
  if (token->info == KW_Return) {
    Node* node = make_unary(ND_Return, expr(&token, token + 1, scopes));
    *rest = expect_eol(token - 1);
    return node;

  } else if (token->info == KW_If) {
    Node* cond = expr(&token, token + 1, scopes);
    Node* then = stmt(&token, token, scopes);
    Node* elseb = nullptr;
    if (token->info == KW_Else) {
      elseb = stmt(&token, token + 1, scopes);
    }
    Node* node = make_if_node(cond, then, elseb);
    *rest = token;
    return node;

  } else if (token->info == KW_While) {
    Node* cond = expr(&token, token + 1, scopes);
    Node* then = stmt(rest, token, scopes);
    Node* node = make_while_node(cond, then);
    return node;

  } else if (token->info == KW_Let) {
    Node* decl = declaration(rest, token + 1, scopes);
    return decl;

  } else if (token->info == PK_LeftBrace) {
    return compound_stmt(rest, token + 1, scopes);
  }
  return expr(rest, token, scopes);
}

// compound-stmt = stmt* "}"
static Node* compound_stmt(Token** rest, Token* token, Scopes* scopes) {
  Node handle = {};
  Node* node_cursor = &handle;
  while (token->info != PK_RightBrace) {
    token = expect_eol(token - 1);
    node_cursor->next = stmt(&token, token, scopes);
    node_cursor = node_cursor->next;
  }
  Node* node = make_unary(ND_Block, handle.next);
  *rest = token + 1;
  return node;
}

// expr = assign
static Node* expr(Token** rest, Token* token, Scopes* scopes) {
  return assign(rest, token, scopes);
}

// assign = equality ("=" assign)?
static Node* assign(Token** rest, Token* token, Scopes* scopes) {
  Node* node = equality(&token, token, scopes);
  if (token->info == PK_Assign) {
    node = make_oper(OP_Asg, node, assign(&token, token + 1, scopes));
  }
  *rest = token;
  return node;
}

// equality = relational ("==" relational | "!=" relational)*
static Node* equality(Token** rest, Token* token, Scopes* scopes) {
  Node* node = relational(&token, token, scopes);

  while (true) {
    if (token->info == PK_Eq) {
      node = make_oper(OP_Eq, node, relational(&token, token + 1, scopes));
    } else if (token->info == PK_NEq) {
      node = make_oper(OP_NEq, node, relational(&token, token + 1, scopes));
    } else {
      *rest = token;
      return node;
    }
  }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node* relational(Token** rest, Token* token, Scopes* scopes) {
  Node* node = add(&token, token, scopes);

  while (true) {
    if (token->info == PK_Lt) {
      node = make_oper(OP_Lt, node, add(&token, token + 1, scopes));
    } else if (token->info == PK_Lte) {
      node = make_oper(OP_Lte, node, add(&token, token + 1, scopes));
    } else if (token->info == PK_Gt) {
      node = make_oper(OP_Gt, node, add(&token, token + 1, scopes));
    } else if (token->info == PK_Gte) {
      node = make_oper(OP_Gte, node, add(&token, token + 1, scopes));
    } else {
      *rest = token;
      return node;
    }
  }
}

// add = mul ("+" mul | "-" mul)*
static Node* add(Token** rest, Token* token, Scopes* scopes) {
  Node* node = mul(&token, token, scopes);

  while (true) {
    if (token->info == PK_Add) {
      // node = make_add(node, mul(&token, token + 1), token);
      node = make_oper(OP_Add, node, mul(&token, token + 1, scopes));
    } else if (token->info == PK_Sub) {
      // node = make_sub(node, mul(&token, token + 1), token);
      node = make_oper(OP_Sub, node, mul(&token, token + 1, scopes));
    } else {
      *rest = token;
      return node;
    }
  }
}

// mul = unary ("*" unary | "/" unary)*
static Node* mul(Token** rest, Token* token, Scopes* scopes) {
  Node* node = unary(&token, token, scopes);

  while (true) {
    if (token->info == PK_Mul) {
      node = make_oper(OP_Mul, node, unary(&token, token + 1, scopes));
    } else if (token->info == PK_Div) {
      node = make_oper(OP_Div, node, unary(&token, token + 1, scopes));
    } else {
      *rest = token;
      return node;
    }
  }
}

// unary = ("+" | "-") unary
//       | primary
static Node* unary(Token** rest, Token* token, Scopes* scopes) {
  if (token->info == PK_Add) {
    return unary(rest, token + 1, scopes);
  } else if (token->info == PK_Sub) {
    return make_unary(ND_Negation, unary(rest, token + 1, scopes));
  }
  return primary(rest, token, scopes);
}

// funcall = ident "(" (equality ("," equality).*)? ")"
static Node* fn_call(Token** rest, Token* token, Scopes* scopes) {
  StrView name = token->pos;
  token = expect_ident(token);
  token = expect_info(token, PK_LeftParen);

  Node* args = parse_list(&token, token, scopes, PK_RightParen, equality);
  Node* call = make_call_node(name, args);
  *rest = expect_info(token, PK_RightParen);
  return call;
}

// primary = "(" expr ")"
//         | ident ( func-args? )
//         | ident*
//         | ident&
//         | num | flt | str
static Node* primary(Token** rest, Token* token, Scopes* scopes) {
  if (token->kind == TK_Punct) {
    if (token->info == PK_LeftParen) {
      Node* node = expr(&token, token + 1, scopes);
      *rest = expect_info(token, PK_RightParen);
      return node;

    } else if (token->info == PK_LeftBrace) {
      return stmt(rest, token, scopes);

    } else if (token->info == PK_LeftBracket) {
      if (token[1].info == PK_RightBracket) {
        Node* type = make_array_type(make_basic_type(TP_Undf), 0);
        Node* node = make_pointer_value(type, nullptr);
        *rest = token + 2;
        return node;
      }
      Node* list = parse_list(rest, token, scopes, PK_RightBracket, unary);
      TypeKind first_type = list->value.type->type.kind;
      usize size = 0;
      for (Node* value = list; value != nullptr; value = value->next) {
        if (value->value.type->type.kind != first_type) {
          error_tok(token, "Non uniform type found in initializer");
        } else {
          size += 1;
        }
      }
      Node* type = make_array_type(list->value.type, size);
      Node* node = make_pointer_value(type, list);
      return node;
    }

  } else if (token->kind == TK_Ident) {
    if (token[1].info == PK_LeftParen) {
      return fn_call(rest, token, scopes);
    }

    Node* var = find_variable(token, scopes);
    if (var == nullptr) {
      error_tok(token, "Variable not found in scope");

    } else if (token[1].info == PK_AddrOf) {
      *rest = token + 2;
      return make_unary(ND_Addr, var);

    } else if (token[1].info == PK_Deref) {
      *rest = token + 2;
      return make_unary(ND_Deref, var);

    } else if (token[1].info == PK_LeftBracket) {
      Node* index = expr(&token, token + 2, scopes);
      *rest = token + 1;
      return make_oper(OP_ArrIdx, var, index);
    }
    *rest = token + 1;
    return var;

  } else if (token->kind == TK_IntLiteral) {
    Node* type = make_numeric_type(TP_SInt, 32);
    Node* node = make_basic_value(type, token->pos);
    *rest = token + 1;
    return node;

  } else if (token->kind == TK_FltLiteral) {
    Node* type = make_numeric_type(TP_Flt, 64);
    Node* node = make_basic_value(type, token->pos);
    *rest = token + 1;
    return node;

  } else if (token->kind == TK_StrLiteral) {
    Node* node = make_str_value(token->pos);
    *rest = token + 1;
    return node;
  }
  error_tok(token, "Expected an expression");
}
