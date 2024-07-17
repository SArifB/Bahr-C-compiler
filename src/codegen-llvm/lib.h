#pragma once
#include <utility/mod.h>

typedef struct CompileOptions CompileOptions;
struct CompileOptions {
  StrView input_string;
  rcstr input_file_name;
  rcstr output_file_name;
  u32 verbosity_level;
};

extern void compile_string(CompileOptions opts);
