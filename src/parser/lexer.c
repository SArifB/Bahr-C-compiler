#include <parser/ctors.h>
#include <parser/lexer.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility/mod.h>

DEFINE_VECTOR(Token, parser_alloc, parser_dealloc)

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

static bool skippable(char ref) {
  return ref == ' ' || ref == '\t' || ref == '\r' || ref == '\f';
}

static bool is_numliteral(char ref) {
  return (ref >= '0' && ref <= '9') || ref == '.' || ref == '_';
}

static bool is_number(char ref) {
  return ref >= '0' && ref <= '9';
}

static bool is_ident(char ref) {
  return (ref >= 'a' && ref <= 'z') || (ref >= 'A' && ref <= 'Z') || ref == '_';
}

static bool is_charnum(char ref) {
  return (ref >= 'a' && ref <= 'z') || (ref >= 'A' && ref <= 'Z') ||
         (ref >= '0' && ref <= '9') || ref == '_';
}

static const char pnct_table[][4] = {
  "(",  ")",  "{",  "}",  "[",  "]", "+=", "-=", "*=", "/=",
  "->", "=>", ":=", "&&", "||", "!", "?",  "<<", ">>", "==",
  "!=", "<=", ">=", "<",  ">",  "&", "|",  "~",  "=",  "+",
  "-",  "*",  "/",  ",",  ";",  ".", ":",  "#",  "%",  "@",
};

static usize pnct_sizes[sizeof_arr(pnct_table)];

static const enum AddInfo pnct_info_table[sizeof_arr(pnct_table)] = {
  PK_LeftParen,    PK_RightParen,    PK_LeftBracket, PK_RightBracket,
  PK_LeftSqrBrack, PK_RightSqrBrack, PK_AddAssign,   PK_SubAssign,
  PK_MulAssign,    PK_DivAssign,     PK_RightArrow,  PK_FatRightArrow,
  PK_Declare,      PK_And,           PK_Or,          PK_Neg,
  PK_Question,     PK_ShiftLeft,     PK_ShiftRight,  PK_Eq,
  PK_NEq,          PK_Lte,           PK_Gte,         PK_Lt,
  PK_Gt,           PK_Ampersand,     PK_Pipe,        PK_BitNeg,
  PK_Assign,       PK_Add,           PK_Sub,         PK_Mul,
  PK_Div,          PK_Comma,         PK_SemiCol,     PK_Dot,
  PK_Colon,        PK_Hash,          PK_Percent,     PK_AddrOf,
};

static const char kwrd_table[][8] = {
  "ext",  "pub", "let",   "fn",    "use", "if",
  "else", "for", "while", "match", "ret",
};

static usize kwrd_sizes[sizeof_arr(kwrd_table)];

static const enum AddInfo kwrd_info_table[sizeof_arr(kwrd_table)] = {
  KW_Ext,  KW_Pub, KW_Let,   KW_Fn,    KW_Use,    KW_If,
  KW_Else, KW_For, KW_While, KW_Match, KW_Return,
};

static const char flt_table[][4] = {
  "f16", "bf16", "f32", "f64", "f128",
};

static usize flt_sizes[sizeof_arr(flt_table)] = {
  3, 4, 3, 3, 4,
};

static const enum AddInfo flt_info_table[sizeof_arr(flt_table)] = {
  AD_F16Type, AD_BF16Type, AD_F32Type, AD_F64Type, AD_F128Type,
};

typedef struct OptNumIdx OptNumIdx;
struct OptNumIdx {
  usize size;
  bool flt;
  bool some;
};

static OptNumIdx try_get_num_lit(cstr iter) {
  if (is_numliteral(*iter) != true) {
    return (OptNumIdx){0};
  }
  usize size = 0;
  bool flt = false;
  for (; is_numliteral(iter[size]) == true; ++size) {
    if (iter[size] == '.' && flt == true) {
      error_at(&iter[size], "More than one '.' found");
    } else if (iter[size] == '.' && flt == false) {
      flt = true;
    }
  }
  return (OptNumIdx){
    .size = size,
    .flt = flt,
    .some = true,
  };
}

typedef struct OptIdx OptIdx;
struct OptIdx {
  usize size;
  bool some;
};

static OptIdx try_get_str_lit(cstr iter) {
  if (*iter != '\"') {
    return (OptIdx){0};
  }
  usize size = 1;
  /*   return *ref == '\"' || (*ref == '\"' && *(ref - 1) == '\\'); */
  // while (() == false) {
  while (iter[size] != '\"' || (iter[size] != '\"' && iter[size - 1] != '\\')) {
    size += 1;
  }
  return (OptIdx){
    .size = size,
    .some = true,
  };
}

static OptIdx try_get_char_lit(cstr iter) {
  if (*iter != '\'') {
    return (OptIdx){0};
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
  if (is_ident(*iter) == false) {
    return (OptIdx){0};
  }
  for (usize i = 0; i < sizeof_arr(kwrd_table); ++i) {
    if (strncmp(iter, kwrd_table[i], kwrd_sizes[i]) == 0 && //
      is_charnum(iter[kwrd_sizes[i]]) == false) {
      return (OptIdx){
        .size = i,
        .some = true,
      };
    }
  }
  return (OptIdx){0};
}

static OptIdx try_get_fltt(cstr iter) {
  if (*iter != 'f' && *iter != 'b') {
    return (OptIdx){0};
  }
  for (usize i = 0; i < sizeof_arr(flt_table); ++i) {
    if (strncmp(iter, flt_table[i], flt_sizes[i]) == 0 && //
      is_charnum(iter[flt_sizes[i]]) == false) {
      return (OptIdx){
        .size = i,
        .some = true,
      };
    }
  }
  return (OptIdx){0};
}

static OptIdx try_get_intt(cstr iter, char comp) {
  if (*iter != comp) {
    return (OptIdx){0};
  }
  usize size = 1;
  while (is_number(iter[size])) {
    size += 1;
  }
  if (is_charnum(iter[size]) == true) {
    return (OptIdx){0};
  }
  return (OptIdx){
    .size = size,
    .some = true,
  };
}

static OptIdx try_get_ident(cstr iter) {
  usize size = 0;
  while (is_charnum(iter[size]) == true) {
    size += 1;
  }
  if (size == 0) {
    return (OptIdx){0};
  }
  return (OptIdx){
    .size = size,
    .some = true,
  };
}

static OptIdx try_get_punct(cstr iter) {
  for (usize i = 0; i < sizeof_arr(pnct_table); ++i) {
    if (strncmp(iter, pnct_table[i], pnct_sizes[i]) == 0) {
      return (OptIdx){
        .size = i,
        .some = true,
      };
    }
  }
  return (OptIdx){0};
}

#define tokens_push(...) Token_vector_push(&tokens, ((Token){__VA_ARGS__}))

TokenVector* lex_string(const StrView view) {
  TokenVector* tokens = Token_vector_make(64);
  if (tokens == nullptr) {
    eputs("No tokens found");
    exit(1);
  }
  current_input = view;

  for (usize i = 0; i < sizeof_arr(pnct_table); ++i) {
    pnct_sizes[i] = strlen(pnct_table[i]);
  }
  for (usize i = 0; i < sizeof_arr(kwrd_table); ++i) {
    kwrd_sizes[i] = strlen(kwrd_table[i]);
  }

  const char* iter = view.ptr;
  while (iter != view.ptr + view.size) {
    if (skippable(*iter) == true) {
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
        tokens_push(.kind = TK_FltLiteral, .pos = {iter, opt_num.size}, );
      } else {
        tokens_push(.kind = TK_NumLiteral, .pos = {iter, opt_num.size}, );
      }
      iter += opt_num.size;
      continue;
    }

    OptIdx opt;
    /// String
    opt = try_get_str_lit(iter);
    if (opt.some == true) {
      tokens_push(.kind = TK_StrLiteral, .pos = {iter + 1, opt.size - 1}, );
      iter += opt.size + 1;
      continue;
    }

    /// Char
    opt = try_get_char_lit(iter);
    if (opt.some == true) {
      tokens_push(.kind = TK_CharLiteral, .pos = {iter + 1, opt.size}, );
      iter += opt.size + 1;
      continue;
    }

    /// Keyword
    opt = try_get_kwrd(iter);
    if (opt.some == true) {
      tokens_push(
          .kind = TK_Keyword, .info = kwrd_info_table[opt.size],
          .pos = {iter, kwrd_sizes[opt.size]},
      );
      iter += kwrd_sizes[opt.size];
      continue;
    }

    /// Floating point
    opt = try_get_fltt(iter);
    if (opt.some == true) {
      tokens_push(
          .kind = TK_Ident, .info = flt_info_table[opt.size],
          .pos = {iter, flt_sizes[opt.size]},
      );
      iter += flt_sizes[opt.size];
      continue;
    }

    /// Signed integer
    opt = try_get_intt(iter, 'i');
    if (opt.some == true) {
      tokens_push(
          .kind = TK_Ident, .info = AD_SIntType, .pos = {iter, opt.size},
      );
      iter += opt.size;
      continue;
    }

    /// Unsigned integer
    opt = try_get_intt(iter, 'u');
    if (opt.some == true) {
      tokens_push(
          .kind = TK_Ident, .info = AD_UIntType, .pos = {iter, opt.size},
      );
      iter += opt.size;
      continue;
    }

    /// Ident
    opt = try_get_ident(iter);
    if (opt.some == true) {
      tokens_push(.kind = TK_Ident, .pos = {iter, opt.size}, );
      iter += opt.size;
      continue;
    }

    /// Punct
    opt = try_get_punct(iter);
    if (opt.some == true) {
      tokens_push(
          .kind = TK_Punct, .info = pnct_info_table[opt.size],
          .pos = {iter, pnct_sizes[opt.size]},
      );
      iter += pnct_sizes[opt.size];
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
      case TK_NumLiteral:   eprintf("NumLiteral:%*s", 3, "");  break;
      case TK_FltLiteral:   eprintf("FltLiteral:%*s", 3, "");  break;
      case TK_StrLiteral:   eprintf("StrLiteral:%*s", 3, "");  break;
      case TK_CharLiteral:  eprintf("CharLiteral:%*s", 2, ""); break;
      case TK_Keyword:      eprintf("Keyword:%*s", 6, "");     break;
      case TK_Punct:        eprintf("Punct:%*s", 8, "");       break;
    }  // clang-format on
    eputw(itr->pos);
  }
}
