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
extern LLVMValueRef codegen_generate(Codegen* cdgn, Node* prog);
extern void codegen_dispose(Codegen* cdgn);

extern void codegen_set_alloc(fn(void*(usize)) ctor);
extern void codegen_set_dealloc(fn(void(void*)) dtor);
