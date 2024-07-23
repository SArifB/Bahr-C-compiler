#pragma once
#include <utility/mod.h>

typedef struct CLIOptions CLIOptions;
struct CLIOptions {
  StrView compile;
  StrView output;
  const i32 verbosity;
};

typedef const rcstr* const restrict argv_t;
CLIOptions cli_options_parse(isize argc, argv_t argv);
