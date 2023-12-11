#pragma once
#include "codegen/mod.h"
#include <llvm-c/ExecutionEngine.h>

LLVMExecutionEngineRef engine_make(Codegen* cdgn);
void engine_shutdown(LLVMExecutionEngineRef engine);
