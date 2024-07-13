#pragma once
#include <parser/mod.h>
#include <utility/mod.h>

typedef struct CodegenOptions CodegenOptions;
struct CodegenOptions {
  cstr name;
  Node* prog;
  cstr output;
  bool verbose;
};

extern void codegen_generate(CodegenOptions opts);
