#include <cli/inputfile.h>
#include <codegen-llvm/lib.h>
#include <stdio.h>
#include <utility/mod.h>

#include "cli/cl-input-parse.h"

i32 main(i32 argc, rcstr argv[]) {
  CLParserOutput cl_input = parse_cl_input(argc, argv);
  eputn("compile: ");
  eputw(cl_input.compile);
  if (cl_input.compile.length != 0) {
  }
  eputn("output: ");
  eputw(cl_input.output);
  if (cl_input.output.length != 0) {
  }
  eprintln("verbosity: %d", cl_input.verbosity);
  if (cl_input.verbosity != 0) {
  }

  // if (argc < 3) {
  //   eprintln("Usage: %s <input file> <output file>", argv[0]);
  //   return 1;
  // }
  // InputFile input = input_file(argv[1]);

  // compile_string((CompileOptions){
  //   // .verbosity_level = 1,
  //   .output_file_name = argv[2],
  //   .input_file_name = input.name,
  //   .input_string = {
  //     .pointer = input.file,
  //     .length = input.length,
  //   },
  // });

  // input_free(input);
  // return 0;
}
