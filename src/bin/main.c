#include <bin/input.h>
#include <codegen/mod.h>
#include <lexer/mod.h>
#include <parser/mod.h>
#include <stdio.h>
#include <stdlib.h>
#include <utility/mod.h>

i32 main(i32 argc, cstr argv[]) {
  // Get String
  if (argc != 2) {
    eprintf("Usage: %s <input file>", argv[0]);
    return 1;
  }
  StrView content = input_file(argv[1]);
  eputw(content);
  eputs("\n-----------------------------------------------");

  // Lex String
  TokenVector* tokens = lex_string(content);
  lexer_print(tokens);
  eputs("\n-----------------------------------------------");

  // Parse lexer
  Node* prog = parse_lexer(tokens);
  free(tokens);
  input_free(content);
  print_ast(prog);
  eputs("\n-----------------------------------------------");

  // Generate code
  Codegen* cdgn = codegen_make("some_code");
  codegen_generate(cdgn, prog);
  free_ast();
  codegen_dispose(cdgn);
  eputs("\n-----------------------------------------------");
  return 0;
}
