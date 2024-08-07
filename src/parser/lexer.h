#pragma once
#include <utility/mod.h>
#include <utility/vec.h>

typedef enum TokenKind : u16 {
  TK_Ident,
  TK_BasicType,
  TK_IntLiteral,
  TK_FltLiteral,
  TK_StrLiteral,
  TK_CharLiteral,
  TK_Keyword,
  TK_Punct,
} TokenKind;

typedef enum AddInfo : u16 {
  AD_None,

  // Integer types
  AD_SIntType,
  AD_UIntType,

  // Floating point types
  AD_F16Type,
  AD_BF16Type,
  AD_F32Type,
  AD_F64Type,
  AD_F128Type,

  // Punct
  PK_LeftParen,
  PK_RightParen,
  PK_LeftBrace,
  PK_RightBrace,
  PK_LeftBracket,
  PK_RightBracket,
  PK_Ellipsis,
  PK_DotDot,
  PK_Deref,
  PK_AddrOf,
  PK_AddAssign,
  PK_SubAssign,
  PK_MulAssign,
  PK_DivAssign,
  PK_ModAssign,
  PK_AndAssign,
  PK_OrAssign,
  PK_XorAssign,
  PK_DeclAssign,
  PK_LeftShftAsgn,
  PK_RightShftAsgn,
  PK_LeftArrow,
  PK_RightArrow,
  PK_RightFatArrow,
  PK_And,
  PK_Or,
  PK_Not,
  PK_Question,
  PK_LeftShift,
  PK_RightShift,
  PK_Eq,
  PK_NEq,
  PK_Lte,
  PK_Gte,
  PK_Lt,
  PK_Gt,
  PK_Ampersand,
  PK_Pipe,
  PK_Xor,
  PK_Negation,
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
} AddInfo;

typedef struct Token Token;
struct Token {
  TokenKind kind : 15;
  bool is_eol : 1;
  AddInfo info;
  u32 len;
  rcstr pos;
};
DEFINE_VECTOR(Token)

#define strview_from_token(token)                    \
  (StrView) {                                        \
    .pointer = (token)->pos, .length = (token)->len, \
  }

extern TokenVector* lex_string(StrView view);
extern void lexer_print(TokenVector* lexer);

unreturning extern void error(rcstr fmt, ...);
unreturning extern void error_at(StrView view, rcstr loc, rcstr fmt, ...);
unreturning extern void error_tok(StrView view, Token* tok, rcstr fmt, ...);
