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
  va_start(ap);
  evprintf(fmt, ap);
  eputs();
  exit(1);
}

unreturning static void verror_at(cstr location, cstr fmt, va_list ap) {
  i32 line_num = 1;
  for (cstr cursor = current_input.itr; cursor < location; ++cursor) {
    if (*cursor == '\n') {
      line_num += 1;
    }
  }
  cstr input = current_input.itr;
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
  va_start(ap);
  verror_at(location, fmt, ap);
}

unreturning void error_tok(Token* token, cstr fmt, ...) {
  va_list ap;
  va_start(ap);
  verror_at(token->pos.itr, fmt, ap);
}

static bool skippable(cstr itr) {
  return *itr == ' ' || *itr == '\t' || *itr == '\r' || *itr == '\f';
}

static bool is_numliteral(cstr ref) {
  return (*ref >= '0' && *ref <= '9') || *ref == '.' || *ref == '_';
}

static bool is_number(cstr ref) {
  return *ref >= '0' && *ref <= '9';
}

static bool is_str_lit_char(cstr ref) {
  return *ref == '\"' || (*ref == '\"' && *(ref - 1) == '\\');
}

static bool is_ident(cstr ref) {
  return (*ref >= 'a' && *ref <= 'z') || (*ref >= 'A' && *ref <= 'Z') ||
         *ref == '_';
}

static bool is_charnum(cstr ref) {
  return (*ref >= 'a' && *ref <= 'z') || (*ref >= 'A' && *ref <= 'Z') ||
         (*ref >= '0' && *ref <= '9') || *ref == '_';
}

static bool is_keyword(cstr itr, cstr ref, usize size) {
  return strncmp(itr, ref, size) == 0 &&
         ((itr[size] >= 'a' && itr[size] <= 'z') ||
          (itr[size] >= 'A' && itr[size] <= 'Z')) == false;
}

static const char pnct_table[][4] = {
  "(",  ")",  "{",  "}",  "[",  "]", "+=", "-=", "*=", "/=",
  "->", "=>", ":=", "&&", "||", "!", "?",  "<<", ">>", "==",
  "!=", "<=", ">=", "<",  ">",  "&", "|",  "~",  "=",  "+",
  "-",  "*",  "/",  ",",  ";",  ".", ":",  "#",  "%",  "@",
};

static usize pnct_sizes[sizeof_arr(pnct_table)];

static const AddInfo pnct_info_table[sizeof_arr(pnct_table)] = {
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

static const AddInfo kwrd_info_table[sizeof_arr(kwrd_table)] = {
  KW_Ext,  KW_Pub, KW_Let,   KW_Fn,    KW_Use,    KW_If,
  KW_Else, KW_For, KW_While, KW_Match, KW_Return,
};

static const char flt_table[][4] = {
  "f16", "bf16", "f32", "f64", "f128",
};

static usize flt_sizes[sizeof_arr(flt_table)] = {
  3, 4, 3, 3, 4,
};

static const AddInfo flt_info_table[sizeof_arr(flt_table)] = {
  AD_F16Type, AD_BF16Type, AD_F32Type, AD_F64Type, AD_F128Type,
};

static bool is_punct(cstr ref) {
  for (usize i = 0; i < sizeof_arr(pnct_table); ++i) {
    if (strncmp(ref, ref, pnct_sizes[i]) == 0) {
      return true;
    }
  }
  return false;
}

#define tokens_push(...) Token_vector_push(&tokens, ((Token){__VA_ARGS__}))

TokenVector* lex_string(const StrView view) {
  TokenVector* tokens = Token_vector_make(0);
  current_input = view;

  for (usize i = 0; i < sizeof_arr(pnct_table); ++i) {
    pnct_sizes[i] = strlen(pnct_table[i]);
  }
  for (usize i = 0; i < sizeof_arr(kwrd_table); ++i) {
    kwrd_sizes[i] = strlen(kwrd_table[i]);
  }

  cstr itr = view.itr;
  for (; itr != view.sen; ++itr) {
    if (skippable(itr) == true) {
      // Skip
      /// End of line
    } else if (*itr == '\n') {
      if (tokens->length != 0) {
        tokens->buffer[tokens->length - 1].is_eol = true;
      }

      /// Comment
    } else if (strncmp(itr, "//", sizeof("//") - 1) == 0) {
      while (*itr != '\n') {
        itr += 1;
      }

      /// Number
    } else if (is_numliteral(itr) == true) {
      cstr tmp_sen = itr + 1;
      bool flt = false;
      while (is_numliteral(tmp_sen) == true) {
        if (*tmp_sen == '.' && flt == true) {
          error_at(tmp_sen, "More than one '.' found");
        } else if (*tmp_sen == '.' && flt == false) {
          flt = true;
        }
        tmp_sen += 1;
      }
      if (flt == true) {
        tokens_push(
            .kind = TK_NumLiteral, .info = AD_F64Type, .pos = {itr, tmp_sen},
        );
      } else {
        tokens_push(
            .kind = TK_NumLiteral, .info = AD_SIntType, .pos = {itr, tmp_sen},
        );
      }
      itr = tmp_sen - 1;

      /// String
    } else if (*itr == '\"') {
      cstr tmp_sen = itr + 1;
      while (is_str_lit_char(tmp_sen) == false) {
        tmp_sen += 1;
      }
      tokens_push(.kind = TK_StrLiteral, .pos = {itr + 1, tmp_sen}, );
      itr = tmp_sen;

      /// Char
    } else if (*itr == '\'') {
      cstr tmp_sen = itr + 1;
      while (*tmp_sen != '\'') {
        tmp_sen += 1;
      }
      tokens_push(.kind = TK_CharLiteral, .pos = {itr + 1, tmp_sen}, );
      itr = tmp_sen;

      /// Keyword
    } else if (is_ident(itr) == true) {
      bool kw = false;
      for (usize i = 0; i < sizeof_arr(kwrd_table); ++i) {
        if (is_keyword(itr, kwrd_table[i], kwrd_sizes[i])) {
          tokens_push(
              .kind = TK_Keyword, .info = kwrd_info_table[i],
              .pos = {itr, itr + kwrd_sizes[i]},
          );
          itr += kwrd_sizes[i] - 1;
          kw = true;
          break;
        }
      }
      if (kw == true) {
        continue;
      }

      /// Floating point
      bool flt = false;
      for (usize i = 0; i < sizeof_arr(kwrd_table); ++i) {
        if (strncmp(itr, flt_table[i], flt_sizes[i]) == 0 && //
          is_charnum(itr + flt_sizes[i]) == false) {
          tokens_push(
              .kind = TK_Ident, .info = flt_info_table[i],
              .pos = {itr, itr + flt_sizes[i]},
          );
          itr += flt_sizes[i] - 1;
          flt = true;
          break;
        }
      }
      if (flt == true) {
        continue;
      }

      cstr tmp_sen = itr + 1;
      /// Signed integer
      if (*itr == 'i') {
        while (is_number(tmp_sen)) {
          tmp_sen += 1;
        }
        tokens_push(
            .kind = TK_Ident, .info = AD_SIntType, .pos = {itr, tmp_sen},
        );
        itr = tmp_sen - 1;
        continue;

        /// Unsigned integer
      } else if (*itr == 'u') {
        while (is_number(tmp_sen)) {
          tmp_sen += 1;
        }
        tokens_push(
            .kind = TK_Ident, .info = AD_UIntType, .pos = {itr, tmp_sen},
        );
        itr = tmp_sen - 1;
        continue;
      }

      /// Ident
      while (is_charnum(tmp_sen) == true) {
        tmp_sen += 1;
      }
      tokens_push(.kind = TK_Ident, .pos = {itr, tmp_sen}, );
      itr = tmp_sen - 1;

      /// Punct
    } else if (is_punct(itr) == true) {
      for (usize i = 0; i < sizeof_arr(pnct_table); ++i) {
        if (strncmp(itr, pnct_table[i], pnct_sizes[i]) == 0) {
          tokens_push(
              .kind = TK_Punct, .info = pnct_info_table[i],
              .pos = {itr, itr + pnct_sizes[i]},
          );
          itr += pnct_sizes[i] - 1;
          break;
        }
      }

    } else {
      error_at(itr, "Invalid token");
    }
  }

  tokens_push(.kind = TK_EOF, .pos = {itr, itr}, );
  return tokens;
}

void print_str_view(StrView view) {
  eprintf("%.*s\n", (i32)(view.sen - view.itr), view.itr);
}

void lexer_print(TokenVector* tokens) {
  Token* itr = tokens->buffer;
  Token* sen = tokens->buffer + tokens->length;
  for (; itr != sen; ++itr) {
    switch (itr->kind) {  // clang-format off
      case TK_Ident:        eprintf("TK_Ident:%*s", 8, "");       break;
      case TK_NumLiteral:   eprintf("TK_NumLiteral:%*s", 3, "");  break;
      case TK_StrLiteral:   eprintf("TK_StrLiteral:%*s", 3, "");  break;
      case TK_CharLiteral:  eprintf("TK_CharLiteral:%*s", 2, ""); break;
      case TK_Keyword:      eprintf("TK_Keyword:%*s", 6, "");     break;
      case TK_Punct:        eprintf("TK_Punct:%*s", 8, "");       break;
      case TK_EOF:          eprintf("%s","TK_EOF");               break;
    }  // clang-format on
    print_str_view(itr->pos);
  }
}
