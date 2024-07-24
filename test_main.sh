#!/usr/bin/env bash

# This generates an object file and links it to the C standard library
# It should then print the hello world example
# The project needs to be built first

./build/bahrc -c test/src/test2.bh -o test/out2/main.o -v 1
cd test/out2
gcc -fuse-ld=mold -o main main.o
./main command
