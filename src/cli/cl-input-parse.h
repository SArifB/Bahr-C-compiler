#pragma once
#include "utility/mod.h"

typedef struct CLParserOutput CLParserOutput;
struct CLParserOutput {
  StrView compile;
  StrView output;
  const i32 verbosity;
};

CLParserOutput parse_cl_input(i32 argc, rcstr argv[]);
