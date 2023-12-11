#pragma once
#include <utility/mod.h>
#include <utility/vec.h>

typedef struct Lexer Lexer;
typedef struct Token Token;
typedef enum TokenKind TokenKind;
typedef enum AddInfo AddInfo;

enum TokenKind {
  TK_Ident,
  TK_NumLiteral,
  TK_StrLiteral,
  TK_Keyword,
  TK_Punct,
  TK_EOF,
};

enum AddInfo {
  PK_None,

  // Punct
  PK_RightArrow,
  PK_LeftParen,
  PK_RightParen,
  PK_LeftBracket,
  PK_RightBracket,
  PK_LeftSqrBrack,
  PK_RightSqrBrack,
  PK_PlusEq,
  PK_MinusEq,
  PK_DivEq,
  PK_MulEq,
  PK_Plus,
  PK_Minus,
  PK_Div,
  PK_Mul,
  PK_Eq,
  PK_Colon,
  PK_SemiCol,
  PK_Dot,
  PK_DoubleDot,

  // Keyword
  KW_Pub,
  KW_Let,
  KW_Fn,
  KW_Use,
};

struct Token {
  TokenKind kind;
  AddInfo info;
  StrView pos;
};
DECLARE_VECTOR(Token)

extern TokenVector* lex_string(const StrView view);
// extern void dispose_lexer(Lexer lexer);
extern void lexer_print(TokenVector* lexer);
extern bool equals(cstr itr, cstr ref);
extern bool char_equals(cstr itr, cstr ref);
