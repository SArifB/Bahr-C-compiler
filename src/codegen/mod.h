#pragma once
// #include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Types.h>
#include <parser/mod.h>
#include <utility/mod.h>

typedef struct Codegen Codegen;
struct Codegen {
  LLVMContextRef ctx;
  LLVMModuleRef mod;
  LLVMBuilderRef bldr;
};

extern LLVMValueRef codegen_generate(cstr name, Node* prog);
