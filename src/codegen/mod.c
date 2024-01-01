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

DECLARE_VECTOR(LLVMValueRef)
DEFINE_VECTOR(LLVMValueRef, malloc, free)

DECLARE_VECTOR(LLVMTypeRef)
DEFINE_VECTOR(LLVMTypeRef, malloc, free)

DECLARE_VECTOR(cstr)
DEFINE_VECTOR(cstr, malloc, free)

typedef struct {
  LLVMValueRef function;
  LLVMTypeRef type;
  cstr name;
} DeclFn;

DECLARE_VECTOR(DeclFn)
DEFINE_VECTOR(DeclFn, malloc, free)

typedef struct {
  LLVMValueRef variable;
  cstr name;
  bool is_ptr;
} DeclVar;

DECLARE_VECTOR(DeclVar)
DEFINE_VECTOR(DeclVar, malloc, free)

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

static DeclFnVector* decl_fns = nullptr;

void codegen_dispose(Codegen* cdgn) {
  free(decl_fns);
  LLVMShutdown();
  LLVMDisposeBuilder(cdgn->bldr);
  LLVMDisposeModule(cdgn->mod);
  LLVMContextDispose(cdgn->ctx);
  free(cdgn);
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

static DeclVarVector* decl_vars = nullptr;

static DeclFn* get_decl_fn(StrView name) {
  usize size = name.sen - name.itr;
  for (usize i = 0; i < decl_fns->length; ++i) {
    if (
      strncmp(name.itr, decl_fns->buffer[i].name, size) == 0
      && decl_fns->buffer[i].name[size] == '\0') {
      return &decl_fns->buffer[i];
    }
  }
  return nullptr;
}

static DeclVar* get_decl_var(StrView name) {
  usize size = name.sen - name.itr;
  for (usize i = 0; i < decl_vars->length; ++i) {
    if (
      strncmp(name.itr, decl_vars->buffer[i].name, size) == 0
      && decl_vars->buffer[i].name[size] == '\0') {
      return &decl_vars->buffer[i];
    }
  }
  return nullptr;
}

static bool is_integer(Node* node) {
  return node->value.kind == TP_SInt || node->value.kind == TP_UInt;
}

static LLVMTypeRef get_type(LLVMContextRef ctx, Node* type) {
  if (type->kind == ND_Variable) {
    return get_type(ctx, type->unary);

  } else if (type->kind == ND_ArgVar) {
    return get_type(ctx, type->declaration.type);

  } else if (is_integer(type)) {
    return LLVMIntTypeInContext(ctx, type->value.bit_width);

  } else if (type->value.kind == TP_Flt) {
    if (type->value.bit_width == 15) {
      return LLVMBFloatTypeInContext(ctx);
    } else if (type->value.bit_width == 16) {
      return LLVMHalfTypeInContext(ctx);
    } else if (type->value.bit_width == 32) {
      return LLVMFloatTypeInContext(ctx);
    } else if (type->value.bit_width == 64) {
      return LLVMDoubleTypeInContext(ctx);
    } else if (type->value.bit_width == 128) {
      return LLVMFP128TypeInContext(ctx);
    }

  } else if (type->value.kind == TP_Ptr) {
    return LLVMPointerType(get_type(ctx, type->value.type), 0);
  }
    eputs("ND_Value type unimplemented");
    exit(1);
}

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
static LLVMValueRef codegen_call(
  Codegen* cdgn, Node* node, LLVMValueRef function
);

LLVMValueRef codegen_generate(Codegen* cdgn, Node* prog) {
  for (Node* func = prog; func != nullptr; func = func->next) {
    unused LLVMValueRef ret = codegen_function(cdgn, func);
  }

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

static LLVMValueRef codegen_function(Codegen* cdgn, Node* node) {
  if (decl_fns == nullptr) {
    decl_fns = DeclFn_vector_make(2);
  }
  decl_vars = DeclVar_vector_make(2);

  LLVMTypeRefVector* arg_types = LLVMTypeRef_vector_make(0);
  cstrVector* arg_names = cstr_vector_make(0);

  for (Node* arg = node->function.args; arg != nullptr; arg = arg->next) {
    LLVMTypeRef type = get_type(cdgn->ctx, arg->declaration.type);
    LLVMTypeRef_vector_push(&arg_types, type);
    cstr_vector_push(&arg_names, arg->declaration.name.array);
  }
  LLVMTypeRef ret_type = get_type(cdgn->ctx, node->function.ret_type);

  LLVMTypeRef function_type =
    LLVMFunctionType(ret_type, arg_types->buffer, arg_types->length, false);
  LLVMValueRef function =
    LLVMAddFunction(cdgn->mod, node->function.name.array, function_type);

  DeclFn_vector_push(
    &decl_fns,
    (DeclFn){
      .name = node->function.name.array,
      .function = function,
      .type = function_type,
    }
  );

  if (node->function.body == nullptr) {
    free(arg_types);
    free(arg_names);
    free(decl_vars);
    return function;
  }

  LLVMPositionBuilderAtEnd(
    cdgn->bldr, LLVMAppendBasicBlockInContext(cdgn->ctx, function, "entry")
  );

  for (usize i = 0; i < arg_types->length; ++i) {
    LLVMValueRef decl =
      LLVMBuildAlloca(cdgn->bldr, arg_types->buffer[i], arg_names->buffer[i]);
    LLVMValueRef val = LLVMGetParam(function, i);
    LLVMBuildStore(cdgn->bldr, val, decl);

    DeclVar_vector_push(
      &decl_vars,
      (DeclVar){
        .name = arg_names->buffer[i],
        .variable = decl,
        .is_ptr = true,
      }
    );
  }

  for (Node* branch = node->function.body->unary; branch != nullptr;
       branch = branch->next) {
    unused LLVMValueRef ret = codegen_parse(cdgn, branch, function);
  }

  free(arg_types);
  free(arg_names);
  free(decl_vars);
  return function;
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

  } else if (node->kind == ND_Decl) {
    LLVMTypeRef type = get_type(cdgn->ctx, node->declaration.type);
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
    if (is_integer(node) == true) {
    LLVMTypeRef type = get_type(cdgn->ctx, node->value.type);
    return LLVMConstInt(type, atoi(node->value.basic.array), true);
    } else if (node->value.kind == TP_Str) {
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
    }

  } else if (node->kind == ND_Variable) {
    LLVMTypeRef type = get_type(cdgn->ctx, node->unary->declaration.type);
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
    LLVMTypeRef type = get_type(cdgn->ctx, node->unary);
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
    cdgn->bldr, decl_fn->type, decl_fn->function, call_args->buffer,
    call_args->length, decl_fn->name
  );
  free(call_args);
  return result;
}
