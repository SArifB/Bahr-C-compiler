#include <bahrc/cli-options.h>
#include <bahrc/inputfile.h>
#include <codegen-llvm/lib.h>
#include <stdio.h>
#include <utility/mod.h>

int main(int argc, argv_t argv) {
  CLIOptions opts = cli_options_parse(argc, argv);
  Inputfile file = inputfile_make(opts.compile);

  compile_string((CompileOptions){
    .verbosity_level = opts.verbosity,
    .output_filename = opts.output,
    .input_filename = opts.compile,
    .input_string = file.content,
  });

  inputfile_free(file);
  return 0;
}
