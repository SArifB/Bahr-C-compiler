#include <parser/ctors.h>
#include <parser/lexer.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility/mod.h>

DEFINE_VEC_FNS(Token, malloc, free)

static StrView current_input;

unreturning void error(cstr fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  evprintf(fmt, ap);
  eputs();
  exit(1);
}

unreturning static void verror_at(cstr location, cstr fmt, va_list ap) {
  i32 line_num = 1;
  for (cstr cursor = current_input.ptr; cursor < location; ++cursor) {
    if (*cursor == '\n') {
      line_num += 1;
    }
  }
  cstr input = current_input.ptr;
  cstr line = location;
  while (input < line && line[-1] != '\n') {
    line--;
  }
  cstr end = location;
  while (*end && *end != '\n') {
    end++;
  }
  i32 chars_written = eprintf("%d: ", line_num);
  eprintf("%.*s\n", (i32)(end - line), line);
  i32 position = (i32)(location - line) + chars_written;

  eprintf("%*s", position, "");
  eprintf("%s", "^ ");
  evprintf(fmt, ap);
  eputs();
  exit(1);
}

unreturning void error_at(cstr location, cstr fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(location, fmt, ap);
}

unreturning void error_tok(Token* token, cstr fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(token->pos.ptr, fmt, ap);
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

static const char punct_table[][4] = {
  "<<=", ">>=", "...", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=",
  ":=",  "..",  ".*",  ".&", "<-", "->", "=>", "&&", "||", "<<", ">>",
  "==",  "!=",  "<=",  ">=", "<",  ">",  "(",  ")",  "{",  "}",  "[",
  "]",   "&",   "|",   "!",  "?",  "^",  "~",  "=",  "+",  "-",  "*",
  "/",   ",",   ";",   ".",  ":",  "#",  "%",  "@",
};

static inline usize punct_size(usize idx) {
  if (idx < 3) {   // less than "+=", ...
    return 3;
  }
  if (idx < 26) {  // less than "<", ...
    return 2;
  }
  return 1;
}

static const AddInfo punct_info_table[sizeof_arr(punct_table)] = {
  PK_LeftShftAsgn, PK_RightShftAsgn, PK_Ellipsis,  PK_AddAssign,
  PK_SubAssign,    PK_MulAssign,     PK_DivAssign, PK_ModAssign,
  PK_AndAssign,    PK_OrAssign,      PK_XorAssign, PK_DeclAssign,
  PK_DotDot,       PK_Deref,         PK_AddrOf,    PK_LeftArrow,
  PK_RightArrow,   PK_RightFatArrow, PK_And,       PK_Or,
  PK_LeftShift,    PK_RightShift,    PK_Eq,        PK_NEq,
  PK_Lte,          PK_Gte,           PK_Lt,        PK_Gt,
  PK_LeftParen,    PK_RightParen,    PK_LeftBrace, PK_RightBrace,
  PK_LeftBracket,  PK_RightBracket,  PK_Ampersand, PK_Pipe,
  PK_Not,          PK_Question,      PK_Xor,       PK_Negation,
  PK_Assign,       PK_Add,           PK_Sub,       PK_Mul,
  PK_Div,          PK_Comma,         PK_SemiCol,   PK_Dot,
  PK_Colon,        PK_Hash,          PK_Percent,   PK_AddrOf,
};

#define MAKE_NAME_TABLE(NAME, SIZE) \
  static const char NAME##_table[][SIZE] = {ENTRIES};
#define MAKE_TABLE(TYPE, NAME, NAME2)                                    \
  static const TYPE NAME##_##NAME2##_table[sizeof_arr(NAME##_table)] = { \
    ENTRIES                                                              \
  };

#define ENTRIES        \
  X("ext", KW_Ext)     \
  X("pub", KW_Pub)     \
  X("let", KW_Let)     \
  X("fn", KW_Fn)       \
  X("use", KW_Use)     \
  X("if", KW_If)       \
  X("else", KW_Else)   \
  X("for", KW_For)     \
  X("while", KW_While) \
  X("match", KW_Match) \
  X("ret", KW_Return)

#define X(str, info) str,
MAKE_NAME_TABLE(kwrd, 8)
#undef X

#define X(str, info) (sizeof(str) - 1),
MAKE_TABLE(size_t, kwrd, size)
#undef X

#define X(str, info) info,
MAKE_TABLE(AddInfo, kwrd, info)
#undef X

#undef ENTRIES

// static const char kwrd_table[][8] = {
//   "ext",  "pub", "let",   "fn",    "use", "if",
//   "else", "for", "while", "match", "ret",
// };

// static const usize kwrd_sizes[sizeof_arr(kwrd_table)] = {
//   3, 3, 3, 2, 3, 2, 4, 3, 5, 5, 3,
// };

// static const AddInfo kwrd_info_table[sizeof_arr(kwrd_table)] = {
//   KW_Ext,  KW_Pub, KW_Let,   KW_Fn,    KW_Use,    KW_If,
//   KW_Else, KW_For, KW_While, KW_Match, KW_Return,
// };

// #define FLOAT_ENTRIES
//   X("f16", AD_F16Type)
//   X("bf16", AD_BF16Type)
//   X("f32", AD_F32Type)
//   X("f64", AD_F64Type)
//   X("f128", AD_F128Type)

static const char flt_table[][4] = {
  "f16", "bf16", "f32", "f64", "f128",
};

static const usize flt_sizes[sizeof_arr(flt_table)] = {
  3, 4, 3, 3, 4,
};

static const AddInfo flt_info_table[sizeof_arr(flt_table)] = {
  AD_F16Type, AD_BF16Type, AD_F32Type, AD_F64Type, AD_F128Type,
};

typedef struct OptNumIdx OptNumIdx;
struct OptNumIdx {
  usize size;
  bool flt;
  bool some;
};

static OptNumIdx try_get_num_lit(cstr iter) {
  if (is_number(*iter) != true) {
    return (OptNumIdx){};
  }
  usize size = 0;
  bool flt_found = false;
  for (; is_num_literal(iter[size]) == true; ++size) {
    if (iter[size] == '.' && flt_found == true) {
      error_at(&iter[size], "More than one '.' found");
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
    if (memcmp(iter, flt_table[i], flt_sizes[i]) == 0 && //
      is_char_number(iter[flt_sizes[i]]) == false) {
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
    if (memcmp(iter, punct_table[i], punct_size(i)) == 0) {
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
    OptNumIdx opt_num = try_get_num_lit(iter);
    if (opt_num.some == true) {
      if (opt_num.flt == true) {
        tokens_push({
          .kind = TK_FltLiteral,
          .pos = {iter, opt_num.size},
        });
      } else {
        tokens_push({
          .kind = TK_IntLiteral,
          .pos = {iter, opt_num.size},
        });
      }
      iter += opt_num.size;
      continue;
    }

    OptIdx opt;
    /// String
    opt = try_get_str_lit(iter);
    if (opt.some == true) {
      tokens_push({
        .kind = TK_StrLiteral,
        .pos = {iter + 1, opt.size - 1},
      });
      iter += opt.size + 1;
      continue;
    }

    /// Char
    opt = try_get_char_lit(iter);
    if (opt.some == true) {
      tokens_push({
        .kind = TK_CharLiteral,
        .pos = {iter + 1, opt.size},
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
        .pos = {iter, kwrd_size_table[opt.size]},
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
        .pos = {iter, flt_sizes[opt.size]},
      });
      iter += flt_sizes[opt.size];
      continue;
    }

    /// Signed integer
    opt = try_get_intt(iter, 'i');
    if (opt.some == true) {
      tokens_push({
        .kind = TK_Ident,
        .info = AD_SIntType,
        .pos = {iter, opt.size},
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
        .pos = {iter, opt.size},
      });
      iter += opt.size;
      continue;
    }

    /// Ident
    opt = try_get_ident(iter);
    if (opt.some == true) {
      tokens_push({
        .kind = TK_Ident,
        .pos = {iter, opt.size},
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
        .pos = {iter, punct_size(opt.size)},
      });
      iter += punct_size(opt.size);
      continue;
    }

    error_at(iter, "Invalid token");
  }
  return tokens;
}

void lexer_print(TokenVector* tokens) {
  Token* itr = tokens->buffer;
  Token* sen = tokens->buffer + tokens->length;
  for (; itr != sen; ++itr) {
    switch (itr->kind) {  // clang-format off
      case TK_Ident:        eprintf("Ident:%*s", 8, "");       break;
      case TK_IntLiteral:   eprintf("IntLiteral:%*s", 3, "");  break;
      case TK_FltLiteral:   eprintf("FltLiteral:%*s", 3, "");  break;
      case TK_StrLiteral:   eprintf("StrLiteral:%*s", 3, "");  break;
      case TK_CharLiteral:  eprintf("CharLiteral:%*s", 2, ""); break;
      case TK_Keyword:      eprintf("Keyword:%*s", 6, "");     break;
      case TK_Punct:        eprintf("Punct:%*s", 8, "");       break;
    }  // clang-format on
    eputw(itr->pos);
  }
}
