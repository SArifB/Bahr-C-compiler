#include <cli/cl-input-parse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility/mod.h>
#include <utility/vec.h>

typedef enum ArgOption : u32 {
  AO_None,
  AO_Compile,
  AO_Output,
  AO_Verbosity,
} ArgOption;

typedef enum ArgFindType : u32 {
  AFT_None,
  AFT_String,
  AFT_Number,
} ArgFindType;

typedef struct MutStrView MutStrView;
struct MutStrView {
  rcstr pointer;
  usize length;
};

#define strview_from_mutstrview(VIEW)                  \
  (StrView) {                                          \
    .pointer = (VIEW).pointer, .length = (VIEW).length \
  }

typedef struct ArgFindResult {
  ArgOption option;
  ArgFindType type;
  union {
    MutStrView view;
    isize number;
  };
} ArgFindResult;

DEFINE_VECTOR(ArgFindResult)
DEFINE_VEC_FNS(ArgFindResult, malloc, free)

#define MAKE_LONG_ARG_TABLE \
  static const char long_arg_table[][16] = { ENTRIES };
#define MAKE_ARG_TABLE(TYPE, NAME) \
  static const TYPE NAME##_table[total_args_size] = { ENTRIES };

#define ENTRIES                             \
  X(AO_Compile, AFT_String, "compile", 'c') \
  X(AO_Output, AFT_String, "output", 'o')   \
  X(AO_Verbosity, AFT_Number, "verbosity", 'v')

#define X(OPT, TYPE, LONG, SHORT) LONG,
MAKE_LONG_ARG_TABLE
#undef X

#define total_args_size sizeof_arr(long_arg_table)

#define X(OPT, TYPE, LONG, SHORT) (sizeof(LONG) - 1),
MAKE_ARG_TABLE(usize, long_arg_size)
#undef X

#define X(OPT, TYPE, LONG, SHORT) OPT,
MAKE_ARG_TABLE(ArgOption, arg_opt)
#undef X

#define X(OPT, TYPE, LONG, SHORT) TYPE,
MAKE_ARG_TABLE(ArgFindType, arg_type)
#undef X

#define X(OPT, TYPE, LONG, SHORT) SHORT,
MAKE_ARG_TABLE(char, short_arg)
#undef X

#undef ENTRIES

ArgFindResult argument_parse(usize index, MutStrView option) {
  ArgFindType type = arg_type_table[index];
  if (type == AFT_String) {
    return (ArgFindResult){
      .option = arg_opt_table[index],
      .type = type,
      .view = option,
    };
  } else if (type == AFT_Number) {
    return (ArgFindResult){
      .option = arg_opt_table[index],
      .type = type,
      .number = atoi(option.pointer),
    };
  } else {
    eputs("Invalid argument type found");
    exit(1);
  }
}

#define long_arg_found(IDX, ARG)                                       \
  ((ARG).length - 2) == long_arg_size_table[IDX] &&                    \
    memcmp(                                                            \
      (ARG).pointer + 2, long_arg_table[IDX], long_arg_size_table[IDX] \
    ) == 0

ArgFindResult argument_find(StrView argument, MutStrView option) {
  if (argument.length == 2 && argument.pointer[0] == '-') {
    for (usize i = 0; i < total_args_size; ++i) {
      if (argument.pointer[1] == short_arg_table[i]) {
        return argument_parse(i, option);
      }
    }
  } else if (argument.length > 3 && argument.pointer[0] == '-' &&
             argument.pointer[1] == '-') {
    for (usize i = 0; i < total_args_size; ++i) {
      if (long_arg_found(i, argument)) {
        return argument_parse(i, option);
      }
    }
  }
  return (ArgFindResult){
    .type = AFT_None,
  };
}

typedef struct TMP_CLParserOutput TMP_CLParserOutput;
struct TMP_CLParserOutput {
  MutStrView compile;
  MutStrView output;
  i32 verbosity;
};

#define clpo_from_tmp(out)                             \
  (CLParserOutput) {                                   \
    .compile = strview_from_mutstrview((out).compile), \
    .output = strview_from_mutstrview((out).output),   \
    .verbosity = (out).verbosity,                      \
  }

CLParserOutput argument_build_output(ArgFindResultVector* results) {
  TMP_CLParserOutput out = {};
  for (usize i = 0; i < results->length; ++i) {
    ArgFindResult result = results->buffer[i];
    switch (result.option) {
      case AO_None:
        eputs("Invalid result received");
        exit(1);
        break;
      case AO_Compile:
        out.compile = result.view;
        break;
      case AO_Output:
        out.output = result.view;
        break;
      case AO_Verbosity:
        out.verbosity = (i32)result.number;
        break;
    }
  }
  return clpo_from_tmp(out);
}

CLParserOutput parse_cl_input(i32 argc, rcstr argv[]) {
  ArgFindResultVector* results = ArgFindResult_vector_make(8);
  for (i32 i = 1; i + 1 < argc; i += 2) {
    StrView argument = {
      .pointer = argv[i],
      .length = strlen(argv[i]),
    };
    MutStrView option = {
      .pointer = argv[i + 1],
      .length = strlen(argv[i + 1]),
    };
    ArgFindResult result = argument_find(argument, option);
    if (result.type != AFT_None) {
      ArgFindResult_vector_push(&results, result);
    }
  }
  CLParserOutput out = argument_build_output(results);
  free(results);
  return out;
}
