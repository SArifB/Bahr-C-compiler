#include <arena/mod.h>
#include <bahr/input.h>
#include <codegen/mod.h>
#include <parser/mod.h>
#include <stdio.h>
#include <stdlib.h>
#include <utility/mod.h>

i32 main(i32 argc, cstr argv[]) {
  if (argc < 3) {
    eprintln("Usage: %s <input file> <output file>", argv[0]);
    return 1;
  }
  InputFile input = input_file(argv[1]);

  ParserOutput out = parse_string((ParserOptions){
    // .verbose = true,
    .input = {
      .ptr = input.file,
      .length = input.length,
    },
  });

  codegen_generate((CodegenOptions){
    // .verbose = true,
    .name = input.name,
    .output = argv[2],
    .prog = out.tree,
  });

  input_free(input);
  arena_free(&out.arena);
  return 0;
}
