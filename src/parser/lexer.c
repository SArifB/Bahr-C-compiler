#include <parser/ctors.h>
#include <parser/lexer.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility/mod.h>

DEFINE_VEC_FNS(Token, malloc, free)

unreturning void error(cstr fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  evprintf(fmt, ap);
  eputs();
  exit(1);
}

unreturning static void verror_at(
  StrView view, cstr location, cstr fmt, va_list ap
) {
  cstr input = view.ptr;
  u32 line_num = 1;
  for (cstr cursor = input; cursor < location; ++cursor) {
    if (*cursor == '\n') {
      line_num += 1;
    }
  }
  cstr line = location;
  while (input < line && line[-1] != '\n') {
    line--;
  }
  cstr end = location;
  while (*end && *end != '\n') {
    end++;
  }
  i32 chars_written = eprintf("%u: ", line_num);
  eprintf("%.*s\n", (i32)(end - line), line);
  i32 position = (i32)(location - line) + chars_written;

  eprintf("%*s", position, "");
  eprintf("%s", "^ ");
  evprintf(fmt, ap);
  eputs();
  exit(1);
}

unreturning void error_at(StrView view, cstr location, cstr fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(view, location, fmt, ap);
}

unreturning void error_tok(StrView view, Token* token, cstr fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(view, token->pos, fmt, ap);
}

static inline bool is_skippable(char ref) {
  return ref == ' ' || ref == '\t' || ref == '\r' || ref == '\f';
}

static inline bool is_num_literal(char ref) {
  return (ref >= '0' && ref <= '9') || ref == '.' || ref == '_';
}

static inline bool is_number(char ref) {
  return ref >= '0' && ref <= '9';
}

static inline bool is_char_literal(char ref) {
  return (ref >= 'a' && ref <= 'z') || (ref >= 'A' && ref <= 'Z') || ref == '_';
}

static inline bool is_char_number(char ref) {
  return is_char_literal(ref) || is_number(ref);
}

#define MAKE_NAME_TABLE(NAME, SIZE) \
  static const char NAME##_table[][SIZE] = {ENTRIES};
#define MAKE_TABLE(TYPE, NAME, NAME2)                                    \
  static const TYPE NAME##_##NAME2##_table[sizeof_arr(NAME##_table)] = { \
    ENTRIES                                                              \
  };

#define ENTRIES              \
  X(PK_LeftShftAsgn, "<<=")  \
  X(PK_RightShftAsgn, ">>=") \
  X(PK_Ellipsis, "...")      \
  X(PK_AddAssign, "+=")      \
  X(PK_SubAssign, "-=")      \
  X(PK_MulAssign, "*=")      \
  X(PK_DivAssign, "/=")      \
  X(PK_ModAssign, "%=")      \
  X(PK_AndAssign, "&=")      \
  X(PK_OrAssign, "|=")       \
  X(PK_XorAssign, "^=")      \
  X(PK_DeclAssign, ":=")     \
  X(PK_DotDot, "..")         \
  X(PK_Deref, ".*")          \
  X(PK_AddrOf, ".&")         \
  X(PK_LeftArrow, "<-")      \
  X(PK_RightArrow, "->")     \
  X(PK_RightFatArrow, "=>")  \
  X(PK_And, "&&")            \
  X(PK_Or, "||")             \
  X(PK_LeftShift, "<<")      \
  X(PK_RightShift, ">>")     \
  X(PK_Eq, "==")             \
  X(PK_NEq, "!=")            \
  X(PK_Lte, "<=")            \
  X(PK_Gte, ">=")            \
  X(PK_Lt, "<")              \
  X(PK_Gt, ">")              \
  X(PK_LeftParen, "(")       \
  X(PK_RightParen, ")")      \
  X(PK_LeftBrace, "{")       \
  X(PK_RightBrace, "}")      \
  X(PK_LeftBracket, "[")     \
  X(PK_RightBracket, "]")    \
  X(PK_Ampersand, "&")       \
  X(PK_Pipe, "|")            \
  X(PK_Not, "!")             \
  X(PK_Question, "?")        \
  X(PK_Xor, "^")             \
  X(PK_Negation, "~")        \
  X(PK_Assign, "=")          \
  X(PK_Add, "+")             \
  X(PK_Sub, "-")             \
  X(PK_Mul, "*")             \
  X(PK_Div, "/")             \
  X(PK_Comma, ",")           \
  X(PK_SemiCol, ";")         \
  X(PK_Dot, ".")             \
  X(PK_Colon, ":")           \
  X(PK_Hash, "#")            \
  X(PK_Percent, "%")         \
  X(PK_AddrOf, "@")

#define X(info, str) str,
MAKE_NAME_TABLE(punct, 4)
#undef X

#define X(info, str) (sizeof(str) - 1),
MAKE_TABLE(size_t, punct, size)
#undef X

#define X(info, str) info,
MAKE_TABLE(AddInfo, punct, info)
#undef X

#undef ENTRIES

#define ENTRIES        \
  X(KW_Ext, "ext")     \
  X(KW_Pub, "pub")     \
  X(KW_Let, "let")     \
  X(KW_Fn, "fn")       \
  X(KW_Use, "use")     \
  X(KW_If, "if")       \
  X(KW_Else, "else")   \
  X(KW_For, "for")     \
  X(KW_While, "while") \
  X(KW_Match, "match") \
  X(KW_Return, "ret")

#define X(info, str) str,
MAKE_NAME_TABLE(kwrd, 8)
#undef X

#define X(info, str) (sizeof(str) - 1),
MAKE_TABLE(size_t, kwrd, size)
#undef X

#define X(info, str) info,
MAKE_TABLE(AddInfo, kwrd, info)
#undef X

#undef ENTRIES

#define ENTRIES          \
  X(AD_F16Type, "f16")   \
  X(AD_BF16Type, "bf16") \
  X(AD_F32Type, "f32")   \
  X(AD_F64Type, "f64")   \
  X(AD_F128Type, "f128")

#define X(info, str) str,
MAKE_NAME_TABLE(flt, 4)
#undef X

#define X(info, str) (sizeof(str) - 1),
MAKE_TABLE(size_t, flt, size)
#undef X

#define X(info, str) info,
MAKE_TABLE(AddInfo, flt, info)
#undef X

#undef ENTRIES

typedef struct OptNumIdx OptNumIdx;
struct OptNumIdx {
  usize size;
  bool flt;
  bool some;
};

static OptNumIdx try_get_num_lit(StrView view, cstr iter) {
  if (is_number(*iter) == false) {
    return (OptNumIdx){};
  }
  usize size = 0;
  bool flt_found = false;
  for (; is_num_literal(iter[size]) == true; ++size) {
    if (iter[size] == '.' && flt_found == true) {
      error_at(view, &iter[size], "More than one '.' found");
    } else if (iter[size] == '.' && flt_found == false) {
      flt_found = true;
    }
  }
  return (OptNumIdx){
    .size = size,
    .flt = flt_found,
    .some = true,
  };
}

typedef struct OptIdx OptIdx;
struct OptIdx {
  usize size;
  bool some;
};

static inline bool not_quote_punct(char ref, char prev) {
  return ref != '\"' || (ref != '\"' && prev != '\\');
}

static OptIdx try_get_str_lit(cstr iter) {
  if (*iter != '\"') {
    return (OptIdx){};
  }
  usize size = 1;
  while (not_quote_punct(iter[size], iter[size - 1]) == true) {
    size += 1;
  }
  return (OptIdx){
    .size = size,
    .some = true,
  };
}

static OptIdx try_get_char_lit(cstr iter) {
  if (*iter != '\'') {
    return (OptIdx){};
  }
  usize size = 1;
  while (iter[size] != '\'') {
    size += 1;
  }
  return (OptIdx){
    .size = size,
    .some = true,
  };
}

static OptIdx try_get_kwrd(cstr iter) {
  if (is_char_literal(*iter) == false) {
    return (OptIdx){};
  }
  for (usize i = 0; i < sizeof_arr(kwrd_table); ++i) {
    if (memcmp(iter, kwrd_table[i], kwrd_size_table[i]) == 0 && //
      is_char_number(iter[kwrd_size_table[i]]) == false) {
      return (OptIdx){
        .size = i,
        .some = true,
      };
    }
  }
  return (OptIdx){};
}

static OptIdx try_get_fltt(cstr iter) {
  if (*iter != 'f' && *iter != 'b') {
    return (OptIdx){};
  }
  for (usize i = 0; i < sizeof_arr(flt_table); ++i) {
    if (memcmp(iter, flt_table[i], flt_size_table[i]) == 0 && //
      is_char_number(iter[flt_size_table[i]]) == false) {
      return (OptIdx){
        .size = i,
        .some = true,
      };
    }
  }
  return (OptIdx){};
}

static OptIdx try_get_intt(cstr iter, char comp) {
  if (*iter != comp) {
    return (OptIdx){};
  }
  usize size = 1;
  while (is_number(iter[size]) == true) {
    size += 1;
  }
  if (is_char_literal(iter[size]) == true) {
    return (OptIdx){};
  }
  return (OptIdx){
    .size = size,
    .some = true,
  };
}

static OptIdx try_get_ident(cstr iter) {
  usize size = 0;
  while (is_char_number(iter[size]) == true) {
    size += 1;
  }
  if (size == 0) {
    return (OptIdx){};
  }
  return (OptIdx){
    .size = size,
    .some = true,
  };
}

static OptIdx try_get_punct(cstr iter) {
  for (usize i = 0; i < sizeof_arr(punct_table); ++i) {
    if (memcmp(iter, punct_table[i], punct_size_table[i]) == 0) {
      return (OptIdx){
        .size = i,
        .some = true,
      };
    }
  }
  return (OptIdx){};
}

#define tokens_push(...) Token_vector_push(&tokens, ((Token)__VA_ARGS__))

TokenVector* lex_string(const StrView view) {
  TokenVector* tokens = Token_vector_make(64);
  current_input = view;

  cstr iter = view.ptr;
  while (iter != view.ptr + view.length) {
    if (is_skippable(*iter) == true) {
      iter += 1;
      continue;
    }

    /// End of line
    if (*iter == '\n') {
      if (tokens->length != 0) {
        tokens->buffer[tokens->length - 1].is_eol = true;
      }
      iter += 1;
      continue;
    }

    /// Comment
    if (*iter == '/' && iter[1] == '/') {
      while (*iter != '\n') {
        iter += 1;
      }
      continue;
    }

    /// Number
    OptNumIdx opt_num = try_get_num_lit(view, iter);
    if (opt_num.some == true) {
      if (opt_num.flt == true) {
        tokens_push({
          .kind = TK_FltLiteral,
          .pos = iter,
          .len = opt_num.size,
        });
      } else {
        tokens_push({
          .kind = TK_IntLiteral,
          .pos = iter,
          .len = opt_num.size,
        });
      }
      iter += opt_num.size;
      continue;
    }

    /// String
    opt = try_get_str_lit(iter);
    if (opt.some == true) {
      tokens_push({
        .kind = TK_StrLiteral,
        .pos = iter + 1,
        .len = opt.size - 1,
      });
      iter += opt.size + 1;
      continue;
    }

    /// Char
    opt = try_get_char_lit(iter);
    if (opt.some == true) {
      tokens_push({
        .kind = TK_CharLiteral,
        .pos = iter + 1,
        .len = opt.size,
      });
      iter += opt.size + 1;
      continue;
    }

    /// Keyword
    opt = try_get_kwrd(iter);
    if (opt.some == true) {
      tokens_push({
        .kind = TK_Keyword,
        .info = kwrd_info_table[opt.size],
        .pos = iter,
        .len = kwrd_size_table[opt.size],
      });
      iter += kwrd_size_table[opt.size];
      continue;
    }

    /// Floating point
    opt = try_get_fltt(iter);
    if (opt.some == true) {
      tokens_push({
        .kind = TK_Ident,
        .info = flt_info_table[opt.size],
        .pos = iter,
        .len = flt_size_table[opt.size],
      });
      iter += flt_size_table[opt.size];
      continue;
    }

    /// Signed integer
    opt = try_get_intt(iter, 'i');
    if (opt.some == true) {
      tokens_push({
        .kind = TK_Ident,
        .info = AD_SIntType,
        .pos = iter,
        .len = opt.size,
      });
      iter += opt.size;
      continue;
    }

    /// Unsigned integer
    opt = try_get_intt(iter, 'u');
    if (opt.some == true) {
      tokens_push({
        .kind = TK_Ident,
        .info = AD_UIntType,
        .pos = iter,
        .len = opt.size,
      });
      iter += opt.size;
      continue;
    }

    /// Ident
    opt = try_get_ident(iter);
    if (opt.some == true) {
      tokens_push({
        .kind = TK_Ident,
        .pos = iter,
        .len = opt.size,
      });
      iter += opt.size;
      continue;
    }

    /// Punct
    opt = try_get_punct(iter);
    if (opt.some == true) {
      tokens_push({
        .kind = TK_Punct,
        .info = punct_info_table[opt.size],
        .pos = iter,
        .len = punct_size_table[opt.size],
      });
      iter += punct_size_table[opt.size];
      continue;
    }

    error_at(view, iter, "Invalid token");
  }
  return tokens;
}

void lexer_print(TokenVector* tokens) {
  Token* iter = tokens->buffer;
  Token* sen = tokens->buffer + tokens->length;
  for (; iter != sen; ++iter) {
    switch (iter->kind) {  // clang-format off
      case TK_Ident:        eprintf("Ident:%*s", 8, "");       break;
      case TK_BasicType:    eprintf("BasicType:%*s", 4, "");   break;
      case TK_IntLiteral:   eprintf("IntLiteral:%*s", 3, "");  break;
      case TK_FltLiteral:   eprintf("FltLiteral:%*s", 3, "");  break;
      case TK_StrLiteral:   eprintf("StrLiteral:%*s", 3, "");  break;
      case TK_CharLiteral:  eprintf("CharLiteral:%*s", 2, ""); break;
      case TK_Keyword:      eprintf("Keyword:%*s", 6, "");     break;
      case TK_Punct:        eprintf("Punct:%*s", 8, "");       break;
    }  // clang-format on
    StrView str = { .ptr = iter->pos, .length = iter->len };
    eputw(str);
  }
}
