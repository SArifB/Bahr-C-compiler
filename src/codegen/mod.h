#pragma once
#include <llvm-c/Types.h>
#include <parser/mod.h>
#include <utility/mod.h>

typedef struct Codegen Codegen;
struct Codegen {
  LLVMContextRef ctx;
  LLVMModuleRef mod;
  LLVMBuilderRef bldr;
};

typedef struct CodegenOptions CodegenOptions;
struct CodegenOptions {
  cstr name;
  Node* prog;
  cstr output;
  bool verbose;
};

extern LLVMValueRef codegen_generate(CodegenOptions opts);
