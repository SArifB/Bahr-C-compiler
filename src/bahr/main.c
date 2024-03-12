#include <arena/mod.h>
#include <bahr/input.h>
#include <codegen/mod.h>
#include <parser/mod.h>
#include <stdio.h>
#include <stdlib.h>
#include <utility/mod.h>

i32 main(i32 argc, cstr argv[]) {
  // Get String
  if (argc == 1) {
    eprintln("Usage: %s <input file>", argv[0]);
    return 1;
  }
  InputFile input = input_file(argv[1]);
  // eprintln("%.*s", (i32)input.size, input.file);
  // eputs("\n-----------------------------------------------");

  // Parse String
  // enable_verbosity();
  Node* prog = parse_string((StrView){
    .ptr = input.file,
    .length = input.length,
  });

  // Generate code
  codegen_generate(input.name, prog);
  // eputs("\n-----------------------------------------------");

  // Cleanup owned memory
  input_free(input);
  parser_dealloc();
  return 0;
}
