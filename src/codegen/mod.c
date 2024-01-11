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

static fn(void*(usize)) codegen_alloc = nullptr;
static fn(void(void*)) codegen_dealloc = nullptr;

void codegen_set_alloc(fn(void*(usize)) ctor) {
  codegen_alloc = ctor;
}

void codegen_set_dealloc(fn(void(void*)) dtor) {
  codegen_dealloc = dtor;
}

DECLARE_VECTOR(LLVMValueRef)
DEFINE_VECTOR(LLVMValueRef, codegen_alloc, codegen_dealloc)

DECLARE_VECTOR(LLVMTypeRef)
DEFINE_VECTOR(LLVMTypeRef, codegen_alloc, codegen_dealloc)

DECLARE_VECTOR(cstr)
DEFINE_VECTOR(cstr, codegen_alloc, codegen_dealloc)

typedef struct {
  LLVMValueRef value;
  LLVMTypeRef type;
  cstrVector* arg_names;
  cstr name;
} DeclFn;

DECLARE_VECTOR(DeclFn)
DEFINE_VECTOR(DeclFn, codegen_alloc, codegen_dealloc)

typedef struct {
  LLVMValueRef variable;
  cstr name;
  bool is_ptr;
} DeclVar;

DECLARE_VECTOR(DeclVar)
DEFINE_VECTOR(DeclVar, codegen_alloc, codegen_dealloc)

Codegen* codegen_make(cstr name) {
  Codegen* cdgn = codegen_alloc(sizeof(Codegen));
  if (cdgn == nullptr) {
    eputs("codegen_alloc failed");
    exit(1);
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
  LLVMShutdown();
  LLVMDisposeBuilder(cdgn->bldr);
  LLVMDisposeModule(cdgn->mod);
  LLVMContextDispose(cdgn->ctx);
  codegen_dealloc(cdgn);
}

unreturning static void print_cdgn_err(NodeKind kind) {
  switch (kind) {  // clang-format off
    case ND_None:       eputs("Invalid nodekind: ND_None");      break;
    case ND_Operation:  eputs("Invalid nodekind: ND_Operation"); break;
    case ND_Negation:   eputs("Invalid nodekind: ND_Negation");  break;
    case ND_Return:     eputs("Invalid nodekind: ND_Return");    break;
    case ND_Block:      eputs("Invalid nodekind: ND_Block");     break;
    case ND_Addr:       eputs("Invalid nodekind: ND_Addr");      break;
    case ND_Deref:      eputs("Invalid nodekind: ND_Deref");     break;
    case ND_Type:       eputs("Invalid nodekind: ND_Type");      break;
    case ND_Decl:       eputs("Invalid nodekind: ND_Decl");      break;
    case ND_Value:      eputs("Invalid nodekind: ND_Value");     break;
    case ND_Variable:   eputs("Invalid nodekind: ND_Variable");  break;
    case ND_ArgVar:     eputs("Invalid nodekind: ND_ArgVar");    break;
    case ND_Function:   eputs("Invalid nodekind: ND_Function");  break;
    case ND_If:         eputs("Invalid nodekind: ND_If");        break;
    case ND_While:      eputs("Invalid nodekind: ND_While");     break;
    case ND_Call:       eputs("Invalid nodekind: ND_Call");      break;
  }  // clang-format on
  exit(1);
}

static DeclFnVector* decl_fns = nullptr;
static DeclVarVector* decl_vars = nullptr;

static DeclFn* get_decl_fn(StrView name) {
  usize size = name.size;
  for (usize i = 0; i < decl_fns->length; ++i) {
    if (
      strncmp(name.ptr, decl_fns->buffer[i].name, size) == 0
      && decl_fns->buffer[i].name[size] == '\0') {
      return &decl_fns->buffer[i];
    }
  }
  return nullptr;
}

static DeclVar* get_decl_var(StrView name) {
  usize size = name.size;
  for (usize i = 0; i < decl_vars->length; ++i) {
    if (
      strncmp(name.ptr, decl_vars->buffer[i].name, size) == 0
      && decl_vars->buffer[i].name[size] == '\0') {
      return &decl_vars->buffer[i];
    }
  }
  return nullptr;
}

static bool is_integer(Node* node) {
  return node->type.kind == TP_SInt || node->type.kind == TP_UInt;
}

static LLVMValueRef codegen_reg_fns(Codegen* cdgn, Node* node);
static LLVMValueRef codegen_function(Codegen* cdgn, Node* node);
static LLVMValueRef codegen_parse(
  Codegen* cdgn, Node* node, LLVMValueRef function
);
static LLVMBasicBlockRef codegen_parse_block(
  Codegen* cdgn, Node* node, LLVMValueRef function, cstr name
);
static LLVMValueRef codegen_oper(
  Codegen* cdgn, Node* node, LLVMValueRef function
);
static LLVMValueRef codegen_value(Codegen* cdgn, Node* node);
static LLVMValueRef codegen_call(
  Codegen* cdgn, Node* node, LLVMValueRef function
);
static LLVMTypeRef codegen_type(Codegen* cdgn, Node* node);

LLVMValueRef codegen_generate(Codegen* cdgn, Node* prog) {
  decl_fns = DeclFn_vector_make(8);
  for (Node* func = prog; func != nullptr; func = func->next) {
    unused LLVMValueRef ret = codegen_reg_fns(cdgn, func);
  }
  for (Node* func = prog; func != nullptr; func = func->next) {
    unused LLVMValueRef ret = codegen_function(cdgn, func);
  }
  codegen_dealloc(decl_fns);

  str str_mod = LLVMPrintModuleToString(cdgn->mod);
  if (str_mod == nullptr) {
    eputs("failed to create string representation of module");
    exit(1);
  }
  printf("%s", str_mod);
  LLVMDisposeMessage(str_mod);

  str error = nullptr;
  LLVMVerifyModule(cdgn->mod, LLVMAbortProcessAction, &error);
  eprintf("%s", error);
  // LLVMPrintModuleToFile(cdgn->mod, "test/out/test.ll", &error);
  // eprintf("%s", error);
  LLVMDisposeMessage(error);

  return nullptr;
}

static LLVMValueRef codegen_reg_fns(Codegen* cdgn, Node* node) {
  LLVMTypeRefVector* arg_types = LLVMTypeRef_vector_make(2);
  cstrVector* arg_names = cstr_vector_make(2);

  for (Node* arg = node->function.args; arg != nullptr; arg = arg->next) {
    LLVMTypeRef type = codegen_type(cdgn, arg->declaration.type);
    LLVMTypeRef_vector_push(&arg_types, type);
    cstr_vector_push(&arg_names, arg->declaration.name.array);
  }
  LLVMTypeRef ret_type = codegen_type(cdgn, node->function.ret_type);

  LLVMTypeRef function_type =
    LLVMFunctionType(ret_type, arg_types->buffer, arg_types->length, false);
  LLVMValueRef function =
    LLVMAddFunction(cdgn->mod, node->function.name.array, function_type);

  DeclFn_vector_push(
    &decl_fns,
    (DeclFn){
      .name = node->function.name.array,
      .value = function,
      .type = function_type,
      .arg_names = arg_names,
    }
  );

  codegen_dealloc(arg_types);
  return function;
}

static LLVMValueRef codegen_function(Codegen* cdgn, Node* node) {
  if (node->function.body == nullptr) {
    return nullptr;
  }
  decl_vars = DeclVar_vector_make(2);
  DeclFn* function = get_decl_fn(view_from(node->function.name));

  usize arg_count = LLVMCountParams(function->value);
  LLVMTypeRefVector* arg_types = LLVMTypeRef_vector_make(arg_count);
  LLVMGetParamTypes(function->type, arg_types->buffer);

  LLVMBasicBlockRef block =
    LLVMAppendBasicBlockInContext(cdgn->ctx, function->value, "entry");
  LLVMPositionBuilderAtEnd(cdgn->bldr, block);

  for (usize i = 0; i < arg_count; ++i) {
    cstr name = function->arg_names->buffer[i];
    LLVMValueRef decl = LLVMBuildAlloca(cdgn->bldr, arg_types->buffer[i], name);
    LLVMValueRef val = LLVMGetParam(function->value, i);
    LLVMBuildStore(cdgn->bldr, val, decl);

    DeclVar_vector_push(
      &decl_vars,
      (DeclVar){
        .name = name,
        .variable = decl,
        .is_ptr = true,
      }
    );
  }

  for (Node* branch = node->function.body->unary; branch != nullptr;
       branch = branch->next) {
    unused LLVMValueRef ret = codegen_parse(cdgn, branch, function->value);
  }

  codegen_dealloc(arg_types);
  codegen_dealloc(function->arg_names);
  codegen_dealloc(decl_vars);
  return function->value;
}

static LLVMValueRef codegen_parse(
  Codegen* cdgn, Node* node, LLVMValueRef function
) {
  if (node->kind == ND_Operation) {
    return codegen_oper(cdgn, node, function);

  } else if (node->kind == ND_Negation) {
    LLVMValueRef zero =
      LLVMConstInt(LLVMInt32TypeInContext(cdgn->ctx), 0, false);
    LLVMValueRef value = codegen_parse(cdgn, node->unary, function);
    return LLVMBuildSub(cdgn->bldr, zero, value, "neg");

  } else if (node->kind == ND_Return) {
    LLVMValueRef val = codegen_parse(cdgn, node->unary, function);
    return LLVMBuildRet(cdgn->bldr, val);

  } else if (node->kind == ND_Type) {
    eputs("Raw ND_Type unimplemented");
    exit(1);

  } else if (node->kind == ND_Decl) {
    LLVMTypeRef type = codegen_type(cdgn, node->declaration.type);
    LLVMValueRef decl =
      LLVMBuildAlloca(cdgn->bldr, type, node->declaration.name.array);
    LLVMValueRef val = codegen_parse(cdgn, node->declaration.value, function);
    LLVMBuildStore(cdgn->bldr, val, decl);
    DeclVar_vector_push(
      &decl_vars,
      (DeclVar){
        .variable = decl,
        .name = node->declaration.name.array,
      }
    );
    return decl;

  } else if (node->kind == ND_Value) {
    return codegen_value(cdgn, node);

  } else if (node->kind == ND_Variable) {
    LLVMTypeRef type = codegen_type(cdgn, node->unary->declaration.type);
    DeclVar* decl_var = get_decl_var(view_from(node->unary->declaration.name));
    if (decl_var == nullptr) {
      eputs("ND_Variable not found");
      exit(1);
    }
    // if (decl_var->is_ptr ==true) {
    //   return decl_var->variable;
    // }
    LLVMValueRef var = LLVMBuildLoad2(cdgn->bldr, type, decl_var->variable, "");
    return var;

  } else if (node->kind == ND_ArgVar) {
    eputs("Raw ND_ArgVar unimplemented");
    exit(1);

  } else if (node->kind == ND_Block) {
    eputs("Raw ND_Block unimplemented");
    exit(1);

  } else if (node->kind == ND_Addr) {
    eputs("ND_Addr unimplemented");
    exit(1);

  } else if (node->kind == ND_Deref) {
    // ->unary->declaration.type
    LLVMTypeRef type = codegen_type(cdgn, node->unary);
    LLVMValueRef ptr = codegen_parse(cdgn, node->unary, function);
    LLVMValueRef load =
      LLVMBuildLoad2(cdgn->bldr, LLVMPointerType(type, 0), ptr, "");
    return load;

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
    return codegen_call(cdgn, node, function);
  }
  print_cdgn_err(node->kind);
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

static LLVMValueRef codegen_oper(
  Codegen* cdgn, Node* node, LLVMValueRef function
) {
  LLVMValueRef lhs = codegen_parse(cdgn, node->operation.lhs, function);
  LLVMValueRef rhs = codegen_parse(cdgn, node->operation.rhs, function);
  switch (node->operation.kind) {
    case OP_Add:
      return LLVMBuildAdd(cdgn->bldr, lhs, rhs, "add");
    case OP_Sub:
      return LLVMBuildSub(cdgn->bldr, lhs, rhs, "sub");
    case OP_Mul:
      return LLVMBuildMul(cdgn->bldr, lhs, rhs, "mul");
    case OP_Div:
      return LLVMBuildUDiv(cdgn->bldr, lhs, rhs, "div");
    case OP_Eq:
      return LLVMBuildICmp(cdgn->bldr, LLVMIntEQ, lhs, rhs, "eq");
    case OP_NEq:
      return LLVMBuildICmp(cdgn->bldr, LLVMIntNE, lhs, rhs, "neq");
    case OP_Lt:
      return LLVMBuildICmp(cdgn->bldr, LLVMIntSLT, lhs, rhs, "lt");
    case OP_Lte:
      return LLVMBuildICmp(cdgn->bldr, LLVMIntSLE, lhs, rhs, "lte");
    case OP_Gt:
      return LLVMBuildICmp(cdgn->bldr, LLVMIntSGT, lhs, rhs, "gt");
    case OP_Gte:
      return LLVMBuildICmp(cdgn->bldr, LLVMIntSGE, lhs, rhs, "gte");
    case OP_Asg:
      return LLVMBuildStore(cdgn->bldr, rhs, lhs);
    case OP_ArrIdx:
      return nullptr;
  }
  print_cdgn_err(node->kind);
}

static LLVMValueRef codegen_value(Codegen* cdgn, Node* node) {
  if (is_integer(node->value.type) == true) {
    LLVMTypeRef type = codegen_type(cdgn, node->value.type);
    return LLVMConstInt(type, atoi(node->value.basic.array), true);
  } else if (node->value.type->type.kind == TP_Str) {
    LLVMTypeRef type = LLVMArrayType(
      LLVMInt8TypeInContext(cdgn->ctx), node->value.basic.size + 1
    );
    LLVMValueRef str_val = LLVMConstStringInContext(
      cdgn->ctx, node->value.basic.array, node->value.basic.size, false
    );
    LLVMValueRef global_str = LLVMAddGlobal(cdgn->mod, type, ".str");
    LLVMSetInitializer(global_str, str_val);
    LLVMSetLinkage(global_str, LLVMPrivateLinkage);
    LLVMSetUnnamedAddr(global_str, LLVMGlobalUnnamedAddr);
    LLVMSetAlignment(global_str, 1);
    return global_str;
  } else if (node->value.type->type.kind == TP_Arr) {
    // TODO: Implement array value
  }
  print_cdgn_err(node->kind);
}

static LLVMValueRef codegen_call(
  Codegen* cdgn, Node* node, LLVMValueRef function
) {
  DeclFn* decl_fn = get_decl_fn(view_from(node->call_node.name));
  if (decl_fn == nullptr) {
    eputs("ND_Call function not found");
    exit(1);
  }
  LLVMValueRefVector* call_args = LLVMValueRef_vector_make(0);
  for (Node* arg = node->call_node.args; arg != nullptr; arg = arg->next) {
    LLVMValueRef value = codegen_parse(cdgn, arg, function);
    LLVMValueRef_vector_push(&call_args, value);
  }
  LLVMValueRef result = LLVMBuildCall2(
    cdgn->bldr, decl_fn->type, decl_fn->value, call_args->buffer,
    call_args->length, decl_fn->name
  );
  codegen_dealloc(call_args);
  return result;
}

static LLVMTypeRef codegen_type(Codegen* cdgn, Node* node) {
  if (node->kind == ND_Variable) {
    return codegen_type(cdgn, node->unary);

  } else if (node->kind == ND_ArgVar) {
    return codegen_type(cdgn, node->declaration.type);

  } else if (node->kind == ND_Decl) {
    return codegen_type(cdgn, node->declaration.type);

  } else if (node->kind == ND_Value) {
    return codegen_type(cdgn, node->value.type);

  } else if (node->kind == ND_Type) {
    if (is_integer(node)) {
      return LLVMIntTypeInContext(cdgn->ctx, node->type.bit_width);

    } else if (node->type.kind == TP_Flt) {
      if (node->type.bit_width == 15) {
        return LLVMBFloatTypeInContext(cdgn->ctx);
      } else if (node->type.bit_width == 16) {
        return LLVMHalfTypeInContext(cdgn->ctx);
      } else if (node->type.bit_width == 32) {
        return LLVMFloatTypeInContext(cdgn->ctx);
      } else if (node->type.bit_width == 64) {
        return LLVMDoubleTypeInContext(cdgn->ctx);
      } else if (node->type.bit_width == 128) {
        return LLVMFP128TypeInContext(cdgn->ctx);
      }

    } else if (node->type.kind == TP_Ptr) {
      return LLVMPointerType(codegen_type(cdgn, node->type.base), 0);

    } else if (node->type.kind == TP_Arr) {
      return LLVMArrayType(
        codegen_type(cdgn, node->type.array.base), node->type.array.size
      );
    }
    eputs("Type unimplemented");
    exit(1);
  }
  eputs("Node is not a type");
  exit(1);
}

/*
LLVMExecutionEngineRef engine_make(Codegen* cdgn);
void engine_shutdown(LLVMExecutionEngineRef engine);

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
 */
