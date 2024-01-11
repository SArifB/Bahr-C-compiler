#include <arena/mod.h>
#include <bin/input.h>
#include <codegen/mod.h>
#include <parser/mod.h>
#include <stdio.h>
#include <stdlib.h>
#include <utility/mod.h>

static Arena DEFAULT_ARENA = {};

static inline void* default_alloc(usize size) {
  return arena_alloc(&DEFAULT_ARENA, size);
}

static inline void default_dealloc() {
  arena_free(&DEFAULT_ARENA);
}

static inline void void_fn(void*) {
  return;
}

i32 main(i32 argc, cstr argv[]) {
  // Get String
  if (argc != 2) {
    eprintf("Usage: %s <input file>", argv[0]);
    return 1;
  }
  InputFile input = input_file(argv[1]);
  eprintf("%.*s", (i32)input.size, input.file);
  eputs("\n-----------------------------------------------");

  // Set memory management
  parser_set_alloc(default_alloc);
  parser_set_dealloc(void_fn);
  codegen_set_alloc(malloc);
  codegen_set_dealloc(free);

  // Parse String
  enable_verbosity();
  Node* prog = parse_string((StrView){
    .ptr = input.file,
    .size = input.size,
  });

  // Generate code
  Codegen* cdgn = codegen_make(input.name);
  codegen_generate(cdgn, prog);
  eputs("\n-----------------------------------------------");

  // Cleanup owned memory
  input_free(input);
  codegen_dispose(cdgn);
  default_dealloc();
  return 0;
}
