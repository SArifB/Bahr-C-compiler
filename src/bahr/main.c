#include <arena/mod.h>
#include <bahr/input.h>
#include <codegen/mod.h>
#include <parser/mod.h>
#include <stdio.h>
#include <stdlib.h>
#include <utility/mod.h>

i32 main(i32 argc, cstr argv[]) {
  // Get String
  if (argc < 3) {
    eprintln("Usage: %s <input file> <output file>", argv[0]);
    return 1;
  }
  InputFile input = input_file(argv[1]);
  // eprintln("%.*s", (i32)input.length, input.file);
  // eputs("\n-----------------------------------------------");

  // Parse String
  // enable_verbosity();
  ParserOutput out = parse_string((StrView){
    .ptr = input.file,
    .length = input.length,
  });

  // Generate code
  codegen_generate(input.name, out.tree, argv[2]);
  // eputs("\n-----------------------------------------------");

  // Cleanup owned memory
  input_free(input);
  arena_free(&out.arena);
  return 0;
}
