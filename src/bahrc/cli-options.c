#include <bahrc/cli-options.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility/mod.h>
#include <utility/vec.h>

typedef struct MutStrView MutStrView;
struct MutStrView {
  rcstr pointer;
  usize length;
};

#define strview_from_mutstrview(VIEW)                  \
  (StrView) {                                          \
    .pointer = (VIEW).pointer, .length = (VIEW).length \
  }

typedef struct MutCLIOptions MutCLIOptions;
struct MutCLIOptions {
  MutStrView compile;
  MutStrView output;
  i32 verbosity;
};

#define clio_from_mclio(OUT)                           \
  (CLIOptions) {                                       \
    .compile = strview_from_mutstrview((OUT).compile), \
    .output = strview_from_mutstrview((OUT).output),   \
    .verbosity = (OUT).verbosity,                      \
  }

typedef enum ArgFindOption : u32 {
  AO_None,
  AO_Compile,
  AO_Output,
  AO_Verbosity,
} ArgFindOption;

typedef enum ArgFindType : u32 {
  AT_None,
  AT_String,
  AT_Number,
} ArgFindType;

typedef struct ArgFindResult {
  ArgFindOption option;
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
  X(AO_Compile, AT_String, "compile", 'c') \
  X(AO_Output, AT_String, "output", 'o')   \
  X(AO_Verbosity, AT_Number, "verbosity", 'v')

#define X(OPT, TYPE, LONG, SHORT) LONG,
MAKE_LONG_ARG_TABLE
#undef X

#define total_args_size sizeof_arr(long_arg_table)

#define X(OPT, TYPE, LONG, SHORT) (sizeof(LONG) - 1),
MAKE_ARG_TABLE(usize, long_arg_size)
#undef X

#define X(OPT, TYPE, LONG, SHORT) OPT,
MAKE_ARG_TABLE(ArgFindOption, arg_opt)
#undef X

#define X(OPT, TYPE, LONG, SHORT) TYPE,
MAKE_ARG_TABLE(ArgFindType, arg_type)
#undef X

#define X(OPT, TYPE, LONG, SHORT) SHORT,
MAKE_ARG_TABLE(char, short_arg)
#undef X

#undef ENTRIES

typedef enum CLIErrorType : u32 {
  CE_NoArgs,
  CE_NoCompile,
  CE_InvalidArg,
} CLIErrorType;

static const char cli_error_table[][64] = {
  [CE_NoArgs] = "No command options given",
  [CE_NoCompile] = "No input file to be compiled designated",
  [CE_InvalidArg] = "Invalid arg given",
};

#define cli_eputt(TYPE) eprintln("Invalid options: %s", cli_error_table[TYPE])

static ArgFindResult argument_parse(usize index, MutStrView option) {
  ArgFindType type = arg_type_table[index];
  if (type == AT_String) {
    return (ArgFindResult){
      .option = arg_opt_table[index],
      .type = type,
      .view = option,
    };
  } else if (type == AT_Number) {
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

static ArgFindResult argument_find(StrView argument, MutStrView option) {
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
  return (ArgFindResult){};
}

static CLIOptions argument_build_output(ArgFindResultVector* results) {
  MutCLIOptions out = {};
  for (usize i = 0; i < results->length; ++i) {
    ArgFindResult result = results->buffer[i];
    switch (result.option) {
      case AO_Compile:
        out.compile = result.view;
        break;
      case AO_Output:
        out.output = result.view;
        break;
      case AO_Verbosity:
        out.verbosity = (i32)result.number;
        break;
      case AO_None:
        eputs("Invalid result received");
        exit(1);
        break;
    }
  }
  if (out.compile.length == 0) {
    cli_eputt(CE_NoCompile);
    exit(1);
  }
  return clio_from_mclio(out);
}

static const char cli_info_fmt[] =
  "Usage: %s [--compile, -c] <input-file>\n"
  "Options:\n"
  "  [--compile, -c] <input-file: string>: Input file to be compiled\n"
  "  [--output, -o] <output-file: string>: Output file to be written\n"
  "  [--verbosity, -v] <level: number>: Level of verbosity to output messages\n"
  "Additional info:\n"
  "  - Output file defaults to input file with '.o' extension\n"
  "  - Verbosity level does not affect error output and defaults to 0\n";

CLIOptions cli_options_parse(isize argc, argv_t argv) {
  if (argc < 2) {
    cli_eputt(CE_NoArgs);
    eprintf(cli_info_fmt, argv[0]);
    exit(1);
  }
  ArgFindResultVector* results = ArgFindResult_vector_make(8);
  for (usize i = 1; i + 1 < (usize)argc; i += 2) {
    StrView argument = {
      .pointer = argv[i],
      .length = strlen(argv[i]),
    };
    MutStrView option = {
      .pointer = argv[i + 1],
      .length = strlen(argv[i + 1]),
    };
    ArgFindResult result = argument_find(argument, option);
    if (result.type != AT_None) {
      ArgFindResult_vector_push(&results, result);
    } else {
      cli_eputt(CE_InvalidArg);
      eputn("Argument: ");
      eputw(argument);
    }
  }
  CLIOptions out = argument_build_output(results);
  free(results);
  return out;
}
