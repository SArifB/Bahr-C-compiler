#pragma once
#include "utility/mod.h"

typedef struct CLParserOutput CLParserOutput;
struct CLParserOutput {
  StrView compile;
  StrView output;
  const i32 verbosity;
};

typedef const rcstr* const restrict argv_t;
CLParserOutput parse_cl_input(isize argc, argv_t argv);
