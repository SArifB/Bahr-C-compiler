#include <cli/cl-input-parse.h>
#include <cli/inputfile.h>
#include <codegen-llvm/lib.h>
#include <stdio.h>
#include <utility/mod.h>

int main(int argc, argv_t argv) {
  CLParserOutput cl_input = parse_cl_input(argc, argv);
  InputFile input = input_file(cl_input.compile);

  compile_string((CompileOptions){
    .verbosity_level = cl_input.verbosity,
    .output_file_name = cl_input.output,
    .input_file_name = cl_input.compile,
    .input_string = {
      .pointer = input.file,
      .length = input.length,
    },
  });

  input_free(input);
  return 0;
}
