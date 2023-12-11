#include <codegen/mod.h>
#include <engine/mod.h>
#include <lexer/mod.h>
#include <llvm-c/Core.h>
#include <parser/mod.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility/mod.h>

i32 main(i32, cstr[]) {
  // Get String
  FILE* fptr = fopen("test/src/test.bh", "r");
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
  Token* tok_cur = tokens->buffer;
  Node* node = expr(&tok_cur, tok_cur);
  free(tokens);

  print_ast_tree(node);
  eputs("\n-----------------------------------------------");
  // NodeVector* exprs = parse_lexer(tokens);
  // any res = codegen(exprs);

  // Generate code
  Codegen* cdgn = codegen_make("some_code");
  codegen_build_function(cdgn, node);
  // LLVMValueRef value = codegen_generate(cdgn, node);

  LLVMExecutionEngineRef engine = engine_make(cdgn);

  free(ref_str);
  free_ast_tree(node);
  engine_shutdown(engine);
  codegen_dispose(cdgn);
  return 0;
}
