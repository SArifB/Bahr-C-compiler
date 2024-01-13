#!/usr/bin/env fish

# This generates an assembly file and links it to c standard library
# Then it should print hello world
# The project needs to be built first
# Requires: fish, clang, gcc
# Can substitute fish for any shell and gcc for any compiler

./build/bahr test/src/ast_test.bh > test/out2/main.ll
cd test/out2/
llc -relocation-model=pic main.ll
gcc -o main main.s
./main
