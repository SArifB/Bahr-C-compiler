#pragma once
#include <utility/mod.h>

typedef struct CompileOptions CompileOptions;
struct CompileOptions {
  StrView input_string;
  StrView input_filename;
  StrView output_filename;
  u32 verbosity_level;
};

extern void compile_string(CompileOptions opts);
