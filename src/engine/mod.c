#include <engine/mod.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <stdio.h>
#include <stdlib.h>

LLVMExecutionEngineRef engine_make(Codegen* cdgn) {
  str error = nullptr;
  LLVMVerifyModule(cdgn->mod, LLVMAbortProcessAction, &error);
  LLVMDisposeMessage(error);
  // LLVMDumpModule(cdgn->mod);
  str str_mod = LLVMPrintModuleToString(cdgn->mod);
  if (str_mod == nullptr) {
    eputs("failed to create string reperesentation of module");
    exit(1);
  }
  printf("%s\n", str_mod);
  LLVMDisposeMessage(str_mod);

  LLVMExecutionEngineRef engine;
  LLVMLinkInMCJIT();
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();

  error = nullptr;
  if (LLVMCreateExecutionEngineForModule(&engine, cdgn->mod, &error) != 0) {
    eputs("failed to create execution engine");
    exit(1);
  }
  if (error) {
    eprintf("error: %s\n", error);
    exit(1);
  }
  LLVMDisposeMessage(error);

  // isize addr = LLVMGetFunctionAddress(engine, "sum");
  // fn(int(int, int)) func = (any)addr;
  // feprintln(stderr, "Sum val: %d\n", func(x, y));

  // LLVMRemoveModule(engine, cdgn->mod, &cdgn->mod, &error);
  // LLVMDisposeExecutionEngine(engine);
  return engine;
}

void engine_shutdown(LLVMExecutionEngineRef engine) {
  LLVMDisposeExecutionEngine(engine);
  LLVMShutdown();
}
