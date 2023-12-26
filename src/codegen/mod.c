#include <codegen/mod.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <parser/mod.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility/mod.h>
#include <utility/vec.h>

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

DECLARE_VECTOR(LLVMTypeRef)
DEFINE_VECTOR(LLVMTypeRef, malloc, free)

void codegen_dispose(Codegen* cdgn) {
  LLVMShutdown();
  LLVMDisposeBuilder(cdgn->bldr);
  LLVMDisposeModule(cdgn->mod);
  LLVMContextDispose(cdgn->ctx);
  free(cdgn);
}

unreturning static void print_cdgn_err(NodeKind kind) {
  switch (kind) { // clang-format off
    case ND_None:       eputs("Error: Invalid nodekind: ND_None");      break;
    case ND_Operation:  eputs("Error: Invalid nodekind: ND_Operation"); break;
    case ND_Negation:   eputs("Error: Invalid nodekind: ND_Negation");  break;
    case ND_Return:     eputs("Error: Invalid nodekind: ND_Return");    break;
    case ND_Block:      eputs("Error: Invalid nodekind: ND_Block");     break;
    case ND_Addr:       eputs("Error: Invalid nodekind: ND_Addr");      break;
    case ND_Deref:      eputs("Error: Invalid nodekind: ND_Deref");     break;
    case ND_Value:      eputs("Error: Invalid nodekind: ND_Value");     break;
    case ND_Variable:   eputs("Error: Invalid nodekind: ND_Variable");  break;
    case ND_Function:   eputs("Error: Invalid nodekind: ND_Function");  break;
    case ND_If:         eputs("Error: Invalid nodekind: ND_If");        break;
    case ND_While:      eputs("Error: Invalid nodekind: ND_While");     break;
    case ND_Call:       eputs("Error: Invalid nodekind: ND_Call");      break;
  } // clang-format on
  exit(1);
}

// clang-format off
static LLVMValueRef codegen_parse(
  Codegen* cdgn, Node* node, LLVMValueRef function
);
static LLVMBasicBlockRef codegen_parse_block(
  Codegen* cdgn, Node* node, LLVMValueRef function, cstr name
);
static LLVMValueRef codegen_oper(
  Codegen* cdgn, Node* node, LLVMValueRef function
);
static LLVMValueRef codegen_function(
  Codegen* cdgn, Node* node
);
// clang-format on

LLVMValueRef codegen_generate(Codegen* cdgn, Node* prog) {
  for (Node* func = prog; func != nullptr; func = func->next) {
    unused LLVMValueRef ret = codegen_function(cdgn, func);
  }

  str error = nullptr;
  LLVMVerifyModule(cdgn->mod, LLVMAbortProcessAction, &error);
  eprintf("LLVMVerifyModule: %s\n", error);

  str str_mod = LLVMPrintModuleToString(cdgn->mod);
  if (str_mod == nullptr) {
    eputs("failed to create string representation of module");
    exit(1);
  }
  printf("%s\n", str_mod);
  LLVMDisposeMessage(str_mod);

  // LLVMPrintModuleToFile(cdgn->mod, "test/out/out.ll", &error);
  // eprintf("LLVMPrintModuleToFile: %s\n", error);
  LLVMDisposeMessage(error);

  return nullptr;
}

static LLVMValueRef
codegen_parse(Codegen* cdgn, Node* node, LLVMValueRef function) {
  if (node->kind == ND_Operation) {
    return codegen_oper(cdgn, node, function);

  } else if (node->kind == ND_Negation) {
    return codegen_parse(cdgn, node->unary, function);

  } else if (node->kind == ND_Return) {
    LLVMValueRef val = codegen_parse(cdgn, node->unary, function);
    if (node->unary->kind == ND_Variable) {
      LLVMTypeRef type = LLVMInt32TypeInContext(cdgn->ctx);
      LLVMValueRef load = LLVMBuildLoad2(cdgn->bldr, type, val, "ret");
      return LLVMBuildRet(cdgn->bldr, load);
    }
    return LLVMBuildRet(cdgn->bldr, val);
  } else if (node->kind == ND_Value) {
    if (node->value.type == TP_Int) {
      LLVMTypeRef type = LLVMInt32TypeInContext(cdgn->ctx);
      return LLVMConstInt(type, atoi(node->value.basic.array), true);
    }
  } else if (node->kind == ND_Variable) {
    if (node->value.type == TP_Int) {
      LLVMTypeRef type = LLVMInt32TypeInContext(cdgn->ctx);
      return LLVMBuildAlloca(cdgn->bldr, type, node->variable.name.array);
    }
  } else if (node->kind == ND_Block) {
    eputs("Raw ND_Block unimplemented");
    exit(1);

  } else if (node->kind == ND_Addr) {
    eputs("ND_Addr unimplemented");
    exit(1);

  } else if (node->kind == ND_Deref) {
    eputs("ND_Deref unimplemented");
    exit(1);

  } else if (node->kind == ND_Function) {
    eputs("Raw ND_Function unimplemented");
    exit(1);

  } else if (node->kind == ND_If) {
    LLVMValueRef cond = codegen_parse(cdgn, node->if_node.cond, function);
    LLVMBasicBlockRef then =
      codegen_parse_block(cdgn, node->if_node.then, function, "if_then");
    LLVMBasicBlockRef elseb =
      codegen_parse_block(cdgn, node->if_node.elseb, function, "if_elseb");
    LLVMValueRef if_block = LLVMBuildCondBr(cdgn->bldr, cond, then, elseb);
    return if_block;

  } else if (node->kind == ND_While) {
    eputs("ND_While unimplemented");
    exit(1);

  } else if (node->kind == ND_Call) {
    eputs("ND_Call unimplemented");
    exit(1);
  }
  print_cdgn_err(node->kind);
}

static LLVMValueRef codegen_function(Codegen* cdgn, Node* node) {
  LLVMTypeRefVector* arg_types = LLVMTypeRef_vector_make(0);
  if (node->function.args != nullptr) {
    for (Node* arg = node->function.args; arg != nullptr; arg = arg->next) {
      if (arg->variable.type == TP_Int) {
        LLVMTypeRef_vector_push(&arg_types, LLVMInt32TypeInContext(cdgn->ctx));
      } else {
        eputs("Arg type unimplemented");
        exit(1);
      }
    }
  }

  LLVMTypeRef ret_type = nullptr;
  if (node->function.ret_type == TP_Int) {
    ret_type = LLVMInt32TypeInContext(cdgn->ctx);
  } else {
    eputs("Ret type unimplemented");
    exit(1);
  }

  LLVMTypeRef fn_type =
    LLVMFunctionType(ret_type, arg_types->buffer, arg_types->length, false);
  LLVMValueRef fn =
    LLVMAddFunction(cdgn->mod, node->function.name.array, fn_type);
  unused LLVMBasicBlockRef body =
    codegen_parse_block(cdgn, node->function.body, fn, "entry");

  free(arg_types);
  return fn;
}

static LLVMBasicBlockRef codegen_parse_block(
  Codegen* cdgn, Node* node, LLVMValueRef function, cstr name
) {
  LLVMBasicBlockRef entry =
    LLVMAppendBasicBlockInContext(cdgn->ctx, function, name);
  LLVMPositionBuilderAtEnd(cdgn->bldr, entry);

  for (Node* branch = node->unary; branch != nullptr; branch = branch->next) {
    unused LLVMValueRef ret = codegen_parse(cdgn, branch, function);
  }
  return entry;
}

static LLVMValueRef
codegen_oper(Codegen* cdgn, Node* node, LLVMValueRef function) {
  LLVMValueRef lhs = codegen_parse(cdgn, node->operation.lhs, function);
  LLVMValueRef rhs = codegen_parse(cdgn, node->operation.rhs, function);
  switch (node->operation.kind) {
  case OP_Add:
    return LLVMBuildAdd(cdgn->bldr, lhs, rhs, "OP_Add");
  case OP_Sub:
    return LLVMBuildSub(cdgn->bldr, lhs, rhs, "OP_Sub");
  case OP_Mul:
    return LLVMBuildMul(cdgn->bldr, lhs, rhs, "OP_Mul");
  case OP_Div:
    return LLVMBuildUDiv(cdgn->bldr, lhs, rhs, "OP_Div");
  case OP_Eq:
    return LLVMBuildICmp(cdgn->bldr, LLVMIntEQ, lhs, rhs, "OP_Eq");
  case OP_NEq:
    return LLVMBuildICmp(cdgn->bldr, LLVMIntNE, lhs, rhs, "OP_Eq");
  case OP_Lt:
    return LLVMBuildICmp(cdgn->bldr, LLVMIntSLT, lhs, rhs, "OP_Lt");
  case OP_Lte:
    return LLVMBuildICmp(cdgn->bldr, LLVMIntSLE, lhs, rhs, "OP_Lte");
  case OP_Gt:
    return LLVMBuildICmp(cdgn->bldr, LLVMIntSGT, lhs, rhs, "OP_Gt");
  case OP_Gte:
    return LLVMBuildICmp(cdgn->bldr, LLVMIntSGE, lhs, rhs, "OP_Gte");
  case OP_Asg:
    return LLVMBuildStore(cdgn->bldr, rhs, lhs);
  case OP_Decl:
    if (node->operation.rhs->kind == ND_Variable) {
      if (node->value.type == TP_Int) {
        LLVMTypeRef type = LLVMInt32TypeInContext(cdgn->ctx);
        cstr name = node->operation.rhs->variable.name.array;
        LLVMValueRef val = LLVMBuildLoad2(cdgn->bldr, type, rhs, name);
        return LLVMBuildStore(cdgn->bldr, val, lhs);
      }
    } else if (node->operation.rhs->kind == ND_Value) {
      return LLVMBuildStore(cdgn->bldr, rhs, lhs);
    }
  }
  print_cdgn_err(node->kind);
}
