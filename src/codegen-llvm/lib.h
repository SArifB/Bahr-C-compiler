#pragma once
#include <utility/mod.h>

typedef struct CompileOptions CompileOptions;
struct CompileOptions {
  StrView input_string;
  StrView input_file_name;
  StrView output_file_name;
  u32 verbosity_level;
};

extern void compile_string(CompileOptions opts);
