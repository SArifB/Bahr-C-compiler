#pragma once
#include <utility/mod.h>

typedef struct CompileOptions CompileOptions;
struct CompileOptions {
  StrView input_string;
  cstr input_file_name;
  cstr output_file_name;
  u32 verbosity_level;
};

extern void compile_string(CompileOptions opts);
