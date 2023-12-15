#include <codegen/mod.h>
#include <engine/mod.h>
#include <lexer/mod.h>
#include <parser/mod.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility/mod.h>

i32 main(i32 argc, cstr argv[]) {
  // Get String
  if (argc != 2) {
    eprintf("Usage: %s <input file>", argv[0]);
    return 1;
  }
  FILE* fptr = fopen(argv[1], "r");
  if (fptr == nullptr) {
    perror("fopen");
    return 1;
  }
  enum { buf_size = 64 };
  char buffer[buf_size];
  str ref_str = calloc((usize)buf_size * 10, sizeof(char));
  if (ref_str == nullptr) {
    perror("calloc");
    return 1;
  }
  str curs = buffer;
  while ((curs = fgets(curs, buf_size, fptr)) != nullptr) {
    strncat(ref_str, curs, buf_size);
  }
  fclose(fptr);

  // Lex String
  TokenVector* tokens = lex_string((StrView){
    ref_str,
    ref_str + strlen(ref_str),
  });
  lexer_print(tokens);
  eputs("\n-----------------------------------------------");

  // Parse lexer
  Node* node = parse_lexer(tokens);
  free(tokens);
  free(ref_str);
  print_ast_tree(node);
  eputs("\n-----------------------------------------------");

  // Generate code
  Codegen* cdgn = codegen_make("some_code");
  codegen_build_function(cdgn, node);
  // LLVMValueRef value = codegen_generate(cdgn, node);

  LLVMExecutionEngineRef engine = engine_make(cdgn);

  free_ast_tree();
  engine_shutdown(engine);
  codegen_dispose(cdgn);
  return 0;
}
