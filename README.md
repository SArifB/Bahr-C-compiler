# Bahr-C-compiler
A compiler built in C for a C-like language. I will be making some changes to the syntax and it will resemble something more like rust later. At core it will still be as simple as C though. It's still work-in-progress so I only have plus statements work yet. The compiler is based on LLVM and uses it as the backend. I plan on trying to bootstrap this language later.

Heavy inspiration was taken from rui314's [chibicc](https://github.com/rui314/chibicc) and from trinnguyen's [llvm-kaleidoscope-c](https://github.com/trinnguyen/llvm-kaleidoscope-c).

## Project modules and roadmap
The project consists of a main, lexer, parser, codegen, engine, and an arena:

- File reading system still needs a lot of work and I need to develop a more robust directory reader. I plan on making a configurations system aswell. This would need to be seperated into it's own module.
- Lexer is in a somewhat good state and doesn't need extreme improvements.
- Parser needs a lot of work and can only handle the simplest mathematical statements.
- Codegen needs even more work than the parser, and can only handle plus statements. For now I only have a hardcoded test function, that adds function input to the compiled input and returns the result.
- Engine is a seperate module for now as I might choose to make the JIT a central part of the language. This is still under consideration.
- Arena is an arena allocator that I borrowed from Tsoding's [arena](https://github.com/tsoding/arena). I've made extensive configurations to it.

The project lacks processing and creating of types like structs, enums and unions aswell as types like ints and arrays. Most importantly I need to develop declaration and calling of functions.

## Build system
The compiler is built with cmake but I configure the project with cmkr's cmake.toml file. Any changes to project configuration should be done through it. I personally build all modules with clang and all warnings as errors. I also regularly run the project with valgrind to ensure no memory leaks occur.

The project can simply be configured, built and run with:
```sh
cmake -B build
cmake --build build
./build/bin
```
Building the test requires the fish shell, gcc and clang. Fish and gcc can be replaced with any shell or compiler of your choosing. The test project can be ran with:
```sh
./test_run.sh
```
