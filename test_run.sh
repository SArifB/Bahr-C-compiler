#!/usr/bin/env bash

# This generates an object file and links it to an example c file
# It should then print the main functions output
# The project needs to be built first

./build/bahr test/src/test.bh test/out/test.o
cd test/out
gcc -fuse-ld=mold -o test main.c test.o
./test
