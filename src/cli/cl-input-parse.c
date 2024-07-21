#include <cli/cl-input-parse.h>
#include <stdlib.h>
#include <string.h>

#include "utility/mod.h"

static const char long_args[][16] = {
  "compile",
  "output",
  "verbosity",
};

typedef struct CLOption CLOption;
struct CLOption {
  rcstr pointer;
  usize length;
};

CLParserOutput parse_cl_input(i32 argc, rcstr argv[]) {
  CLOption compile = {};
  CLOption output = {};
  i32 verbosity = {};

  for (i32 i = 1; i + 1 < argc; i += 2) {
    StrView argument = {
      .pointer = argv[i],
      .length = strlen(argv[i]),
    };
    StrView option = {
      .pointer = argv[i + 1],
      .length = strlen(argv[i + 1]),
    };
    if (argument.length > 2 && argument.pointer[0] == '-' &&
        argument.pointer[1] == '-') {
      for (usize j = 0; j < sizeof_arr(long_args); ++j) {
        bool is_same =
          memcmp(argument.pointer + 2, long_args[j], argument.length - 2) == 0;
        if (is_same) {
          if (j == 0) {
            compile.pointer = option.pointer;
            compile.length = option.length;
          } else if (j == 1) {
            output.pointer = option.pointer;
            output.length = option.length;
          } else if (j == 2) {
            verbosity = atoi(argv[i + 1]);
          }
        }
      }
    } else if (argument.length > 1 && argument.pointer[0] == '-') {
      if (argument.pointer[1] == 'c') {
        compile.pointer = option.pointer;
        compile.length = option.length;
      } else if (argument.pointer[1] == 'o') {
        output.pointer = option.pointer;
        output.length = option.length;
      } else if (argument.pointer[1] == 'v') {
        verbosity = atoi(argv[i + 1]);
      }
    }
  }
  return (CLParserOutput){
    .compile = {
      .pointer = compile.pointer,
      .length = compile.length,
    },
    .output = {
      .pointer = output.pointer,
      .length = output.length,
    },
    .verbosity = verbosity,
  };
}
