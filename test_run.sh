#!/usr/bin/env fish

# This generates an assembly file and links it to an example c file
# The project needs to be built first
# Requires: fish, clang, gcc
# Can substitute fish for any shell and gcc for any compiler

./build/bahr test/src/test.bh > test/out/test.ll
cd test/out/
llc test.ll
gcc -o test main.c test.s
./test
