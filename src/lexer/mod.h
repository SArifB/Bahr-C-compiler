#pragma once
#include <utility/mod.h>
#include <utility/vec.h>

typedef struct Lexer Lexer;
typedef struct Token Token;
typedef enum TokenKind TokenKind;
typedef enum AddInfo AddInfo;

enum TokenKind {
  TK_EOF,
  TK_Ident,
  TK_NumLiteral,
  TK_StrLiteral,
  TK_CharLiteral,
  TK_Keyword,
  TK_Punct,
};

enum AddInfo {
  AD_None,

  // Punct
  PK_LeftParen,
  PK_RightParen,
  PK_LeftBracket,
  PK_RightBracket,
  PK_LeftSqrBrack,
  PK_RightSqrBrack,
  PK_AddAssign,
  PK_SubAssign,
  PK_MulAssign,
  PK_DivAssign,
  PK_RightArrow,
  PK_FatRightArrow,
  PK_Declare,
  PK_And,
  PK_Or,
  PK_Neg,
  PK_Question,
  PK_ShiftLeft,
  PK_ShiftRight,
  PK_Eq,
  PK_NEq,
  PK_Lte,
  PK_Gte,
  PK_Lt,
  PK_Gt,
  PK_Ampersand,
  PK_Pipe,
  PK_BitNeg,
  PK_Assign,
  PK_Add,
  PK_Sub,
  PK_Mul,
  PK_Div,
  PK_Comma,
  PK_SemiCol,
  PK_Dot,
  PK_Colon,
  PK_Hash,
  PK_Percent,
  PK_AddrOf,

  // Keyword
  KW_Ext,
  KW_Pub,
  KW_Let,
  KW_Fn,
  KW_Use,
  KW_If,
  KW_Else,
  KW_For,
  KW_While,
  KW_Match,
  KW_Return,
};

struct Token {
  TokenKind kind : 16;
  bool is_eol;
  AddInfo info;
  StrView pos;
};
DECLARE_VECTOR(Token)

extern TokenVector* lex_string(const StrView view);
extern void lexer_print(TokenVector* lexer);
