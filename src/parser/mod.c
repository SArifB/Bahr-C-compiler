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

static Node* parse_lexer(LexerOutput lexer, Arena* arena);

ParserOutput parse_string(ParserOptions opts) {
  LexerOutput lexer = lex_string(opts.view);
  if (opts.print_verbose) {
    lexer_print(lexer.tokens);
    eputs("\n-----------------------------------------------");
  }
  Arena arena = {};
  Node* tree = parse_lexer(lexer, &arena);
  if (opts.print_verbose) {
    print_ast(tree);
    eputs("\n-----------------------------------------------");
  }
  free(lexer.tokens);
  free(lexer.views);
  return (ParserOutput){
    .arena = arena,
    .tree = tree,
  };
}

DEFINE_VEC_FNS(Scope, malloc, free)

static Node* find_variable(Token* token, Context cx) {
  Scope* rev_itr = cx.scopes->buffer + cx.scopes->length - 1;
  Scope* rev_sen = cx.scopes->buffer - 1;

  for (; rev_itr != rev_sen; --rev_itr) {
    Node* var = *hashmap_find(rev_itr, token->pos);
    if (var != nullptr) {
      return make_unary(cx.arena, ND_Variable, var);
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
  Token** rest, Token* token, Context cx, AddInfo breaker,
  fn(Node*(Token**, Token*, Context)) callable
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
    cursor->next = callable(&token, token, cx);
    cursor = cursor->next;
  };
  *rest = token;
  return handle.next;
}

static Node* parse_type(Token** rest, Token* token, Context cx);
static Node* extern_function(Token** rest, Token* token, Context cx);
static Node* function(Token** rest, Token* token, Context cx);
static Node* argument(Token** rest, Token* token, Context cx);
static Node* declaration(Token** rest, Token* token, Context cx);
static Node* stmt(Token** rest, Token* token, Context cx);
static Node* compound_stmt(Token** rest, Token* token, Context cx);
static Node* assign(Token** rest, Token* token, Context cx);
static Node* expr(Token** rest, Token* token, Context cx);
static Node* equality(Token** rest, Token* token, Context cx);
static Node* relational(Token** rest, Token* token, Context cx);
static Node* add(Token** rest, Token* token, Context cx);
static Node* mul(Token** rest, Token* token, Context cx);
static Node* unary(Token** rest, Token* token, Context cx);
static Node* primary(Token** rest, Token* token, Context cx);

// parse_type = "[" num "]" | "*" | "i"num | "f"num
static Node* parse_type(Token** rest, Token* token, Context cx) {
  if (token->kind == TK_Punct) {
    if (consume(&token, token, PK_Mul)) {
      return make_pointer_type(cx.arena, parse_type(rest, token, cx));

    } else if (consume(&token, token, PK_LeftBracket)) {
      Node* size = expr(&token, token, cx);
      Node* type = parse_type(rest, expect_info(token, PK_RightBracket), cx);
      if (size->kind == ND_Value) {
        return make_array_type(cx.arena, type, atoi(size->value.basic->array));
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
      Node* type = make_numeric_type(cx.arena, TP_SInt, width);
      *rest = token + 1;
      return type;

    } else if (token->info == AD_UIntType) {
      i32 width = atoi(token->pos.ptr + 1);
      if (width > 128) {
        error_tok(token, "Bit width too wide");
      }
      Node* type = make_numeric_type(cx.arena, TP_UInt, width);
      *rest = token + 1;
      return type;

    } else if (token->info == AD_BF16Type) {
      Node* type = make_numeric_type(cx.arena, TP_Flt, 15);
      *rest = token + 1;
      return type;

    } else if (token->info == AD_F16Type) {
      Node* type = make_numeric_type(cx.arena, TP_Flt, 16);
      *rest = token + 1;
      return type;

    } else if (token->info == AD_F32Type) {
      Node* type = make_numeric_type(cx.arena, TP_Flt, 32);
      *rest = token + 1;
      return type;

    } else if (token->info == AD_F64Type) {
      Node* type = make_numeric_type(cx.arena, TP_Flt, 64);
      *rest = token + 1;
      return type;

    } else if (token->info == AD_F128Type) {
      Node* type = make_numeric_type(cx.arena, TP_Flt, 128);
      *rest = token + 1;
      return type;
    }
  }
  error_tok(token, "Invalid expression");
}

// program = functions*
static Node* parse_lexer(TokenVector* tokens, Arena* arena) {
  Token* token = tokens->buffer;
  Token* sentinel = tokens->buffer + tokens->length;
  Scopes* scopes = Scope_vector_make(8);
  Scope_vector_push(&scopes, hashmap_make(32));
  Context context = {
    scopes,
    arena,
  };

  Node handle = {};
  Node* cursor = &handle;
  while (token != sentinel) {
    if (consume(&token, token, KW_Ext)) {
      cursor->next =
        extern_function(&token, expect_info(token, KW_Fn), context);
      cursor = cursor->next;

    } else {
      cursor->next = function(&token, expect_info(token, KW_Fn), context);
      cursor = cursor->next;
    }
  }
  hashmap_free(scopes->buffer[scopes->length - 1]);
  free(scopes);
  return handle.next;
}

// extern_function = indent "(" args? ")" ":" ret_type
static Node* extern_function(Token** rest, Token* token, Context cx) {
  StrView name = token->pos;
  token = expect_ident(token);
  token = expect_info(token, PK_LeftParen);

  Node* args = parse_list(&token, token, cx, PK_RightParen, argument);
  Node* type = parse_type(rest, token + 1, cx);
  return make_function(cx.arena, type, name, nullptr, args);
}

// function = indent "(" args? ")" ":" ret_type "{" body "}"
static Node* function(Token** rest, Token* token, Context cx) {
  StrView name = token->pos;
  token = expect_ident(token);
  token = expect_info(token, PK_LeftParen);
  Scope_vector_push(&cx.scopes, hashmap_make(8));

  Node* args = parse_list(&token, token, cx, PK_RightParen, argument);
  Node* type = parse_type(&token, token + 1, cx);
  Node* body = compound_stmt(rest, expect_info(token, PK_LeftBrace), cx);

  hashmap_free(cx.scopes->buffer[cx.scopes->length - 1]);
  Scope_vector_pop(cx.scopes);
  return make_function(cx.arena, type, name, body, args);
}

// argument = indent ":" type
static Node* argument(Token** rest, Token* token, Context cx) {
  StrView name = token->pos;
  token = expect_ident(token);
  Node* type = parse_type(rest, token, cx);
  return make_arg_var(cx, type, name);
}

// declaration = indent ":" type "=" expr
static Node* declaration(Token** rest, Token* token, Context cx) {
  StrView name = token->pos;
  token = expect_ident(token);
  Node* type = parse_type(&token, token, cx);
  Token* x = expect_info(token, PK_Assign);
  Node* value = expr(rest, x, cx);
  return make_declaration(cx, type, name, value);
}

// stmt = "return" expr
//      | "if" expr stmt ("else" stmt)?
//      | "while" expr stmt
//      | "let" decl ":" type "=" expr
//      | "{" compound-stmt
//      | expr
static Node* stmt(Token** rest, Token* token, Context cx) {
  if (token->info == KW_Return) {
    Node* node = make_unary(cx.arena, ND_Return, expr(&token, token + 1, cx));
    *rest = expect_eol(token - 1);
    return node;

  } else if (token->info == KW_If) {
    Node* cond = expr(&token, token + 1, cx);
    Node* then = stmt(&token, token, cx);
    Node* elseb = nullptr;
    if (token->info == KW_Else) {
      elseb = stmt(&token, token + 1, cx);
    }
    Node* node = make_if_node(cx.arena, cond, then, elseb);
    *rest = token;
    return node;

  } else if (token->info == KW_While) {
    Node* cond = expr(&token, token + 1, cx);
    Node* then = stmt(rest, token, cx);
    Node* node = make_while_node(cx.arena, cond, then);
    return node;

  } else if (token->info == KW_Let) {
    Node* decl = declaration(rest, token + 1, cx);
    return decl;

  } else if (token->info == PK_LeftBrace) {
    return compound_stmt(rest, token + 1, cx);
  }
  return expr(rest, token, cx);
}

// compound-stmt = stmt* "}"
static Node* compound_stmt(Token** rest, Token* token, Context cx) {
  Node handle = {};
  Node* node_cursor = &handle;
  while (token->info != PK_RightBrace) {
    token = expect_eol(token - 1);
    node_cursor->next = stmt(&token, token, cx);
    node_cursor = node_cursor->next;
  }
  Node* node = make_unary(cx.arena, ND_Block, handle.next);
  *rest = token + 1;
  return node;
}

// expr = assign
static Node* expr(Token** rest, Token* token, Context cx) {
  return assign(rest, token, cx);
}

// assign = equality ("=" assign)?
static Node* assign(Token** rest, Token* token, Context cx) {
  Node* node = equality(&token, token, cx);
  if (token->info == PK_Assign) {
    node = make_oper(cx.arena, OP_Asg, node, assign(&token, token + 1, cx));
  }
  *rest = token;
  return node;
}

// equality = relational ("==" relational | "!=" relational)*
static Node* equality(Token** rest, Token* token, Context cx) {
  Node* node = relational(&token, token, cx);

  while (true) {
    if (token->info == PK_Eq) {
      node =
        make_oper(cx.arena, OP_Eq, node, relational(&token, token + 1, cx));
    } else if (token->info == PK_NEq) {
      node =
        make_oper(cx.arena, OP_NEq, node, relational(&token, token + 1, cx));
    } else {
      *rest = token;
      return node;
    }
  }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node* relational(Token** rest, Token* token, Context cx) {
  Node* node = add(&token, token, cx);

  while (true) {
    if (token->info == PK_Lt) {
      node = make_oper(cx.arena, OP_Lt, node, add(&token, token + 1, cx));
    } else if (token->info == PK_Lte) {
      node = make_oper(cx.arena, OP_Lte, node, add(&token, token + 1, cx));
    } else if (token->info == PK_Gt) {
      node = make_oper(cx.arena, OP_Gt, node, add(&token, token + 1, cx));
    } else if (token->info == PK_Gte) {
      node = make_oper(cx.arena, OP_Gte, node, add(&token, token + 1, cx));
    } else {
      *rest = token;
      return node;
    }
  }
}

// add = mul ("+" mul | "-" mul)*
static Node* add(Token** rest, Token* token, Context cx) {
  Node* node = mul(&token, token, cx);

  while (true) {
    if (token->info == PK_Add) {
      // node = make_add(node, mul(&token, token + 1), token);
      node = make_oper(cx.arena, OP_Add, node, mul(&token, token + 1, cx));
    } else if (token->info == PK_Sub) {
      // node = make_sub(node, mul(&token, token + 1), token);
      node = make_oper(cx.arena, OP_Sub, node, mul(&token, token + 1, cx));
    } else {
      *rest = token;
      return node;
    }
  }
}

// mul = unary ("*" unary | "/" unary)*
static Node* mul(Token** rest, Token* token, Context cx) {
  Node* node = unary(&token, token, cx);

  while (true) {
    if (token->info == PK_Mul) {
      node = make_oper(cx.arena, OP_Mul, node, unary(&token, token + 1, cx));
    } else if (token->info == PK_Div) {
      node = make_oper(cx.arena, OP_Div, node, unary(&token, token + 1, cx));
    } else {
      *rest = token;
      return node;
    }
  }
}

// unary = ("+" | "-") unary
//       | primary
static Node* unary(Token** rest, Token* token, Context cx) {
  if (token->info == PK_Add) {
    return unary(rest, token + 1, cx);
  } else if (token->info == PK_Sub) {
    return make_unary(cx.arena, ND_Negation, unary(rest, token + 1, cx));
  }
  return primary(rest, token, cx);
}

// funcall = ident "(" (equality ("," equality).*)? ")"
static Node* fn_call(Token** rest, Token* token, Context cx) {
  StrView name = token->pos;
  token = expect_ident(token);
  token = expect_info(token, PK_LeftParen);

  Node* args = parse_list(&token, token, cx, PK_RightParen, equality);
  Node* call = make_call_node(cx.arena, name, args);
  *rest = expect_info(token, PK_RightParen);
  return call;
}

// primary = "(" expr ")"
//         | ident ( func-args? )
//         | ident*
//         | ident&
//         | num | flt | str
static Node* primary(Token** rest, Token* token, Context cx) {
  if (token->kind == TK_Punct) {
    if (token->info == PK_LeftParen) {
      Node* node = expr(&token, token + 1, cx);
      *rest = expect_info(token, PK_RightParen);
      return node;

    } else if (token->info == PK_LeftBrace) {
      return stmt(rest, token, cx);

    } else if (token->info == PK_LeftBracket) {
      if (token[1].info == PK_RightBracket) {
        Node* type =
          make_array_type(cx.arena, make_basic_type(cx.arena, TP_Undf), 0);
        Node* node = make_pointer_value(cx.arena, type, nullptr);
        *rest = token + 2;
        return node;
      }
      Node* list = parse_list(rest, token, cx, PK_RightBracket, unary);
      TypeKind first_type = list->value.type->type.kind;
      usize size = 0;
      for (Node* value = list; value != nullptr; value = value->next) {
        if (value->value.type->type.kind != first_type) {
          error_tok(token, "Non uniform type found in initializer");
        } else {
          size += 1;
        }
      }
      Node* type = make_array_type(cx.arena, list->value.type, size);
      Node* node = make_pointer_value(cx.arena, type, list);
      return node;
    }

  } else if (token->kind == TK_Ident) {
    if (token[1].info == PK_LeftParen) {
      return fn_call(rest, token, cx);
    }

    Node* var = find_variable(token, cx);
    if (var == nullptr) {
      error_tok(token, "Variable not found in scope");

    } else if (token[1].info == PK_AddrOf) {
      *rest = token + 2;
      return make_unary(cx.arena, ND_Addr, var);

    } else if (token[1].info == PK_Deref) {
      *rest = token + 2;
      return make_unary(cx.arena, ND_Deref, var);

    } else if (token[1].info == PK_LeftBracket) {
      Node* index = expr(&token, token + 2, cx);
      *rest = token + 1;
      return make_oper(cx.arena, OP_ArrIdx, var, index);
    }
    *rest = token + 1;
    return var;

  } else if (token->kind == TK_IntLiteral) {
    Node* type = make_numeric_type(cx.arena, TP_SInt, 32);
    Node* node = make_basic_value(cx.arena, type, token->pos);
    *rest = token + 1;
    return node;

  } else if (token->kind == TK_FltLiteral) {
    Node* type = make_numeric_type(cx.arena, TP_Flt, 64);
    Node* node = make_basic_value(cx.arena, type, token->pos);
    *rest = token + 1;
    return node;

  } else if (token->kind == TK_StrLiteral) {
    Node* node = make_str_value(cx.arena, token->pos);
    *rest = token + 1;
    return node;
  }
  error_tok(token, "Expected an expression");
}
