#pragma once
#include <lexer/mod.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <utility/mod.h>

unreturning void error(cstr fmt, ...);
unreturning void error_at(cstr loc, cstr fmt, ...);
unreturning void error_tok(Token* tok, cstr fmt, ...);
