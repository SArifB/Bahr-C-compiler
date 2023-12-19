#include <lexer/mod.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DEFINE_VECTOR(Token, malloc, free)

static bool equals(cstr itr, cstr ref) {
  return strncmp(itr, ref, strlen(ref)) == 0;
}

static bool char_equals(cstr itr, cstr ref) {
  // return strncmp(itr, ref, 1) == 0;
  return *itr == *ref;
}

static bool skippable(cstr itr) {
  return char_equals(itr, " ") || char_equals(itr, "\t") ||
         char_equals(itr, "\r") || char_equals(itr, "\n") ||
         char_equals(itr, "\f");
}

static bool is_numliteral(cstr ref) {
  return (*ref >= '0' && *ref <= '9') || *ref == '.' || *ref == '_';
}

static bool is_str_lit_char(cstr ref) {
  return *ref == '\"' || (*ref == '\"' && *(ref - 1) == '\\');
}

static bool is_ident(cstr ref) {
  return (*ref >= 'a' && *ref <= 'z') || (*ref >= 'A' && *ref <= 'Z') ||
         *ref == '_';
}

static const char pnct_table[][4] = {
  "(",  ")", "{", "}",  "[",  "]",  "+=", "-=", "*=", "/=", "->", "=>", "&&",
  "||", "!", "?", "<<", ">>", "==", "!=", "<=", ">=", "<",  ">",  "&",  "|",
  "~",  "=", "+", "-",  "*",  "/",  ",",  ";",  ".",  ":",  "#",  "%",  "@",
};

static const AddInfo pnct_info_table[] = {
  PK_LeftParen,    PK_RightParen,   PK_LeftBracket,
  PK_RightBracket, PK_LeftSqrBrack, PK_RightSqrBrack,
  PK_AddAssign,    PK_SubAssign,    PK_MulAssign,
  PK_DivAssign,    PK_RightArrow,   PK_FatRightArrow,
  PK_And,          PK_Or,           PK_Neg,
  PK_Question,     PK_ShiftLeft,    PK_ShiftRight,
  PK_Eq,           PK_NEq,          PK_Lte,
  PK_Gte,          PK_Lt,           PK_Gt,
  PK_BitAnd,       PK_BitOr,        PK_BitNeg,
  PK_Assign,       PK_Add,          PK_Sub,
  PK_Mul,          PK_Div,          PK_Colon,
  PK_SemiCol,      PK_Dot,          PK_DoubleDot,
  PK_Hash,         PK_Percent,      PK_AddrOf,
};

static const char kwrd_table[][8] = {
  "pub", "let", "fn", "use", "if", "else", "for", "while", "match", "return",
};

static const AddInfo kwrd_info_table[] = {
  KW_Pub,  KW_Let, KW_Fn,    KW_Use,   KW_If,
  KW_Else, KW_For, KW_While, KW_Match, KW_Return,
};

static bool is_punct(cstr ref) {
  for (usize i = 0; i < sizeof_arr(pnct_table); ++i) {
    if (equals(ref, pnct_table[i])) {
      return true;
    }
  }
  return false;
}

static bool is_charnum(cstr ref) {
  return (*ref >= 'a' && *ref <= 'z') || (*ref >= 'A' && *ref <= 'Z') ||
         (*ref >= '0' && *ref <= '9') || *ref == '_';
}

TokenVector* lex_string(const StrView view) {
  TokenVector* tokens = Token_vector_make(0);

  cstr itr = view.itr;
  for (; itr != view.sen; ++itr) {
    if (skippable(itr)) {

    } else if (equals(itr, "//")) {
      while (*itr != '\n') {
        itr += 1;
      }

      /// Number
    } else if (is_numliteral(itr)) {
      cstr tmp_sen = itr + 1;
      while (is_numliteral(tmp_sen) == true) {
        tmp_sen += 1;
      }
      Token_vector_push(
        &tokens, (Token){ .kind = TK_NumLiteral, .pos = { itr, tmp_sen } }
      );
      itr = tmp_sen - 1;

      /// String
    } else if (char_equals(itr, "\"")) {
      cstr tmp_sen = itr + 1;
      while (is_str_lit_char(tmp_sen) == false) {
        tmp_sen += 1;
      }
      Token_vector_push(
        &tokens, (Token){ .kind = TK_StrLiteral, .pos = { itr + 1, tmp_sen } }
      );
      itr = tmp_sen;

      /// Char
    } else if (char_equals(itr, "\'")) {
      i32 len = 1;
      if (itr[-1] == '\\') {
        len = 2;
      }
      Token_vector_push(
        &tokens, (Token){ .kind = TK_NumLiteral, .pos = { itr, itr + len } }
      );
      itr += len;

      /// Keyword
    } else if (is_ident(itr) == true) {
      bool kw = false;
      for (usize i = 0; i < sizeof_arr(kwrd_table); ++i) {
        if (equals(itr, kwrd_table[i])) {
          if (itr[strlen(kwrd_table[i])] == ' ') {
          Token_vector_push(
            &tokens, (Token){ .kind = TK_Keyword,
                              .info = kwrd_info_table[i],
                              .pos = { itr, itr + strlen(kwrd_table[i]) } }
          );
            itr += strlen(kwrd_table[i]);
            kw = true;
          break;
          }
        }
      }
      if (kw == true) {
        continue;
      }
      /// Ident
      cstr tmp_sen = itr + 1;
      while (is_charnum(tmp_sen) == true) {
        tmp_sen += 1;
      }
      Token_vector_push(
        &tokens, (Token){ .kind = TK_Ident, .pos = { itr, tmp_sen } }
      );
      itr = tmp_sen - 1;

      /// Punct
    } else if (is_punct(itr) == true) {
      for (usize i = 0; i < sizeof_arr(pnct_table); ++i) {
        if (equals(itr, pnct_table[i])) {
          Token_vector_push(
            &tokens, (Token){ .kind = TK_Punct,
                              .info = pnct_info_table[i],
                              .pos = { itr, itr + strlen(pnct_table[i]) } }
          );
          itr += strlen(pnct_table[i]) - 1;
          break;
        }
      }
    } else {
      eputs("Error: Invalid token at: ");
      while (*itr != '\n') {
        eputc(*itr++);
      }
      exit(1);
    }
  }

  Token_vector_push(&tokens, (Token){ .kind = TK_EOF, .pos = { itr, itr } });
  return tokens;
}

void print_str_view(StrView view) {
  for (cstr itr = view.itr; itr != view.sen; ++itr) {
    eputc(*itr);
  }
  eputc('\n');
}

void lexer_print(TokenVector* tokens) {
  Token* itr = tokens->buffer;
  Token* sen = tokens->buffer + tokens->length;
  for (; itr != sen; ++itr) {
    switch (itr->kind) {
    case TK_Ident:
      eprintf("TK_Ident:%*s", 7, "");
      print_str_view(itr->pos);
      break;
    case TK_NumLiteral:
      eprintf("TK_NumLiteral:%*s", 2, "");
      print_str_view(itr->pos);
      break;
    case TK_StrLiteral:
      eprintf("TK_StrLiteral:%*s", 2, "");
      print_str_view(itr->pos);
      break;
    case TK_Keyword:
      eprintf("TK_Keyword:%*s", 5, "");
      print_str_view(itr->pos);
      break;
    case TK_Punct:
      eprintf("TK_Punct:%*s", 7, "");
      print_str_view(itr->pos);
      break;
    case TK_EOF:
      eputs("TK_EOF");
      break;
    default:
      exit(-4);
    }
  }
}
