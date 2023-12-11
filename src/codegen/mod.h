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

extern Codegen* codegen_make(cstr name);
extern LLVMValueRef codegen_generate(Codegen* cdgn, Node* node);
extern void codegen_dispose(Codegen* cdgn);
extern void codegen_build_function(Codegen* cdgn, Node* node);
