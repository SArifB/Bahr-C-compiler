#include "utility/mod.h"
#include <codegen/mod.h>
// #include <llvm-c/Analysis.h>
// #include <llvm-c/BitWriter.h>
#include <llvm-c/Core.h>
// #include <llvm-c/Target.h>
#include <stdio.h>
#include <stdlib.h>

Codegen* codegen_make(cstr name) {
  Codegen* cdgn = malloc(sizeof(Codegen));
  if (cdgn == nullptr) {
    perror("malloc");
    exit(-1);
  }
  LLVMContextRef ctx = LLVMContextCreate();
  *cdgn = (Codegen){
    .ctx = ctx,
    .mod = LLVMModuleCreateWithNameInContext(name, ctx),
    .bldr = LLVMCreateBuilderInContext(ctx),
  };
  return cdgn;
}

void codegen_dispose(Codegen* cdgn) {
  LLVMDisposeBuilder(cdgn->bldr);
  // LLVMDisposeModule(cdgn->mod);
  LLVMContextDispose(cdgn->ctx);
  free(cdgn);
}

static __attribute((noreturn)) void print_cdgn_err(NodeKind kind) {
  eprintf("Error: Invalid nodekind: %d", kind);
  exit(-3);
}

LLVMValueRef codegen_make_value(Codegen* cdgn, Node* node) {
  if (node->kind == ND_Operation) {
    LLVMValueRef lhs = codegen_make_value(cdgn, node->operation.lhs);
    LLVMValueRef rhs = codegen_make_value(cdgn, node->operation.rhs);
    return LLVMBuildAdd(cdgn->bldr, lhs, rhs, "oper_add");

  } else if (node->kind == ND_Value) {
    if (node->value.kind == TP_Int) {
      LLVMTypeRef type = LLVMInt32TypeInContext(cdgn->ctx);
      return LLVMConstInt(type, node->value.i_num, true);
    }
  }
  print_cdgn_err(node->kind);
}

LLVMValueRef codegen_generate(Codegen* cdgn, Node* node) {
  LLVMValueRef value = codegen_make_value(cdgn, node);
  return value;
}

void codegen_build_function(Codegen* cdgn, Node* node) {
  // LLVMTypeRef param_types[] = {
  //   LLVMInt32TypeInContext(cdgn->ctx),
  //   LLVMInt32TypeInContext(cdgn->ctx),
  // };
  // LLVMTypeRef func_type =
  //   LLVMFunctionType(LLVMInt32TypeInContext(cdgn->ctx), param_types, 2, 0);
  // LLVMValueRef sum = LLVMAddFunction(cdgn->mod, "sum", func_type);

  // LLVMBasicBlockRef entry =
  //   LLVMAppendBasicBlockInContext(cdgn->ctx, sum, "entry");
  // LLVMPositionBuilderAtEnd(cdgn->bldr, entry);

  // LLVMValueRef tmp =
  //   LLVMBuildAdd(cdgn->bldr, LLVMGetParam(sum, 0), LLVMGetParam(sum, 1),
  //   "tmp");
  // LLVMBuildRet(cdgn->bldr, tmp);

  // LLVMTypeRef param_types[] = { LLVMVoidTypeInContext(cdgn->ctx) };
  /*
  LLVMTypeRef func_type =
    LLVMFunctionType(LLVMInt32TypeInContext(cdgn->ctx), nullptr, 0, 0);
  LLVMValueRef void_func = LLVMAddFunction(cdgn->mod, "add_test", func_type);

  LLVMBasicBlockRef entry =
    LLVMAppendBasicBlockInContext(cdgn->ctx, void_func, "entry");
  LLVMPositionBuilderAtEnd(cdgn->bldr, entry);

  LLVMValueRef value = codegen_make_value(cdgn, node);
  LLVMBuildRet(cdgn->bldr, value);
  */

  LLVMTypeRef param_types[] = { LLVMInt32TypeInContext(cdgn->ctx) };
  LLVMTypeRef func_type =
    LLVMFunctionType(LLVMInt32TypeInContext(cdgn->ctx), param_types, 1, false);
  LLVMValueRef add_test = LLVMAddFunction(cdgn->mod, "add_test", func_type);

  LLVMBasicBlockRef entry =
    LLVMAppendBasicBlockInContext(cdgn->ctx, add_test, "entry");
  LLVMPositionBuilderAtEnd(cdgn->bldr, entry);

  LLVMValueRef value = codegen_make_value(cdgn, node);
  LLVMValueRef ret_val =
    LLVMBuildAdd(cdgn->bldr, value, LLVMGetParam(add_test, 0), "ret_val");
  LLVMBuildRet(cdgn->bldr, ret_val);
}
