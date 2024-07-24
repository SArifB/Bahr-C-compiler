#include <codegen-llvm/lib.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Types.h>
#include <parser/mod.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility/mod.h>
#include <utility/vec.h>

typedef struct CodegenOptions CodegenOptions;
struct CodegenOptions {
  StrView input_name;
  StrView output_name;
  Node* tree;
  bool verbose;
};

static void codegen_generate(CodegenOptions opts);

void compile_string(CompileOptions opts) {
  ParserOutput ast = parse_string((ParserOptions){
    .verbose = opts.verbosity_level > 1,
    .input = opts.input_string,
  });

  codegen_generate((CodegenOptions){
    .verbose = opts.verbosity_level > 0,
    .input_name = opts.input_filename,
    .output_name = opts.output_filename,
    .tree = ast.tree,
  });

  arena_free(&ast.arena);
}

DEFINE_VECTOR(LLVMValueRef)
DEFINE_VEC_FNS(LLVMValueRef, malloc, free)

DEFINE_VECTOR(LLVMTypeRef)
DEFINE_VEC_FNS(LLVMTypeRef, malloc, free)

DEFINE_VECTOR(rcstr)
DEFINE_VEC_FNS(rcstr, malloc, free)

typedef struct {
  LLVMValueRef value;
  LLVMTypeRef type;
  rcstrVector* arg_names;
  rcstr name;
} DeclFn;

DEFINE_VECTOR(DeclFn)
DEFINE_VEC_FNS(DeclFn, malloc, free)

typedef struct {
  LLVMValueRef variable;
  rcstr name;
  bool is_ptr;
} DeclVar;

DEFINE_VECTOR(DeclVar)
DEFINE_VEC_FNS(DeclVar, malloc, free)

static DeclFn* get_decl_fn(DeclFnVector* funcs, StrNode* name) {
  for (usize i = 0; i < funcs->length; ++i) {
    if (memcmp(name->array, funcs->buffer[i].name, name->capacity) == 0 &&
        funcs->buffer[i].name[name->capacity] == 0) {
      return &funcs->buffer[i];
    }
  }
  return nullptr;
}

static DeclVar* get_decl_var(DeclVarVector* vars, StrNode* name) {
  for (usize i = 0; i < vars->length; ++i) {
    if (memcmp(name->array, vars->buffer[i].name, name->capacity) == 0 &&
        vars->buffer[i].name[name->capacity] == 0) {
      return &vars->buffer[i];
    }
  }
  return nullptr;
}

typedef struct Codegen Codegen;
struct Codegen {
  LLVMContextRef context;
  LLVMModuleRef module;
  LLVMBuilderRef builder;
};

static Codegen codegen_make(StrView name) {
  LLVMContextRef context = LLVMContextCreate();
  return (Codegen){
    .context = context,
    .module = LLVMModuleCreateWithNameInContext(name.pointer, context),
    .builder = LLVMCreateBuilderInContext(context),
  };
}

static void codegen_dispose(Codegen gen) {
  LLVMShutdown();
  LLVMDisposeBuilder(gen.builder);
  LLVMDisposeModule(gen.module);
  LLVMContextDispose(gen.context);
}

typedef struct CContext CContext;
struct CContext {
  Codegen gen;
  DeclFn* func;
  DeclFnVector* funcs;
  DeclVarVector* vars;
};

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

static bool is_integer(Node* node) {
  return node->type.kind == TP_SInt || node->type.kind == TP_UInt;
}

static LLVMValueRef codegen_reg_fns(CContext cx, Node* node);
static LLVMValueRef codegen_function(CContext cx, Node* node);
static LLVMValueRef codegen_parse(CContext cx, Node* node);
static LLVMValueRef codegen_oper(CContext cx, Node* node);
static LLVMValueRef codegen_value(CContext cx, Node* node);
static LLVMValueRef codegen_call(CContext cx, Node* node);
static LLVMTypeRef codegen_type(CContext cx, Node* node);
static LLVMBasicBlockRef codegen_parse_block(
  CContext cx, Node* node, rcstr name
);

char* alloc_tmp_outname(StrView filename) {
  usize tmp_outname_size = filename.length + 2;
  char* tmp_outname = malloc(tmp_outname_size);
  if (tmp_outname == nullptr) {
    perror("malloc");
    exit(1);
  }
  memcpy(tmp_outname, filename.pointer, filename.length);
  tmp_outname[tmp_outname_size - 2] = '.';
  tmp_outname[tmp_outname_size - 1] = 'o';
  return tmp_outname;
}

void codegen_generate(CodegenOptions opts) {
  if (opts.input_name.pointer[opts.input_name.length] != '\0') {
    eputn("Invalid module name, required to be nullbyte terminated: ");
    eputw(opts.input_name);
    exit(1);
  }
  CContext cx = {
    .gen = codegen_make(opts.input_name),
    .funcs = DeclFn_vector_make(8),
  };

  for (Node* func = opts.tree; func != nullptr; func = func->next) {
    unused LLVMValueRef ret = codegen_reg_fns(cx, func);
  }
  for (Node* func = opts.tree; func != nullptr; func = func->next) {
    unused LLVMValueRef ret = codegen_function(cx, func);
  }
  for (usize i = 0; i < cx.funcs->length; ++i) {
    free(cx.funcs->buffer[i].arg_names);
  }
  free(cx.funcs);

  if (opts.verbose) {
    LLVMDumpModule(cx.gen.module);
    eputs("\n-----------------------------------------------");
  }

  char* message = nullptr;
  bool failed =
    LLVMVerifyModule(cx.gen.module, LLVMAbortProcessAction, &message);
  if (failed == true) {
    eprintf("%s", message);
    exit(1);
  }
  LLVMDisposeMessage(message);

  LLVMInitializeAllTargetInfos();
  LLVMInitializeAllTargets();
  LLVMInitializeAllTargetMCs();
  LLVMInitializeAllAsmPrinters();

  LLVMTargetRef target;
  char* triple = LLVMGetDefaultTargetTriple();
  failed = LLVMGetTargetFromTriple(triple, &target, &message);
  if (failed == true) {
    eprintf("%s", message);
    exit(1);
  }

  LLVMTargetMachineRef machine = LLVMCreateTargetMachine(
    target, triple, "generic", "", LLVMCodeGenLevelDefault, LLVMRelocPIC,
    LLVMCodeModelDefault
  );

  if (opts.output_name.length != 0) {
    failed = LLVMTargetMachineEmitToFile(
      machine, cx.gen.module, opts.output_name.pointer, LLVMObjectFile, &message
    );
  } else {
    char* tmp_outname = alloc_tmp_outname(opts.input_name);
    failed = LLVMTargetMachineEmitToFile(
      machine, cx.gen.module, tmp_outname, LLVMObjectFile, &message
    );
    free(tmp_outname);
  }
  if (failed == true) {
    eprintf("%s", message);
    exit(1);
  }
  LLVMDisposeMessage(triple);
  LLVMDisposeTargetMachine(machine);

  codegen_dispose(cx.gen);
}

static LLVMValueRef codegen_reg_fns(CContext cx, Node* node) {
  LLVMTypeRefVector* arg_types = LLVMTypeRef_vector_make(2);
  rcstrVector* arg_names = rcstr_vector_make(2);

  for (Node* arg = node->function.args; arg != nullptr; arg = arg->next) {
    LLVMTypeRef type = codegen_type(cx, arg->declaration.type);
    LLVMTypeRef_vector_push(&arg_types, type);
    rcstr_vector_push(&arg_names, arg->declaration.name->array);
  }
  LLVMTypeRef ret_type = codegen_type(cx, node->function.ret_type);

  LLVMTypeRef function_type =
    LLVMFunctionType(ret_type, arg_types->buffer, arg_types->length, false);
  LLVMValueRef function =
    LLVMAddFunction(cx.gen.module, node->function.name->array, function_type);

  if (node->function.linkage == LN_Private) {
    LLVMSetLinkage(function, LLVMInternalLinkage);
  }

  DeclFn_vector_push(
    &cx.funcs,
    (DeclFn){
      .name = node->function.name->array,
      .value = function,
      .type = function_type,
      .arg_names = arg_names,
    }
  );

  free(arg_types);
  return function;
}

static LLVMValueRef codegen_function(CContext cx, Node* node) {
  if (node->function.body == nullptr) {
    return nullptr;
  }
  cx.vars = DeclVar_vector_make(8);
  cx.func = get_decl_fn(cx.funcs, node->function.name);

  usize arg_count = LLVMCountParams(cx.func->value);
  LLVMTypeRefVector* arg_types = LLVMTypeRef_vector_make(arg_count);
  LLVMGetParamTypes(cx.func->type, arg_types->buffer);

  LLVMBasicBlockRef block =
    LLVMAppendBasicBlockInContext(cx.gen.context, cx.func->value, "entry");
  LLVMPositionBuilderAtEnd(cx.gen.builder, block);

  for (usize i = 0; i < arg_count; ++i) {
    rcstr name = cx.func->arg_names->buffer[i];
    LLVMValueRef decl =
      LLVMBuildAlloca(cx.gen.builder, arg_types->buffer[i], name);
    LLVMValueRef val = LLVMGetParam(cx.func->value, i);
    LLVMBuildStore(cx.gen.builder, val, decl);

    DeclVar_vector_push(
      &cx.vars,
      (DeclVar){
        .name = name,
        .variable = decl,
        .is_ptr = true,
      }
    );
  }

  for (Node* branch = node->function.body->unary; branch != nullptr;
       branch = branch->next) {
    unused LLVMValueRef ret = codegen_parse(cx, branch);
  }

  free(arg_types);
  free(cx.vars);
  return cx.func->value;
}

static LLVMBasicBlockRef codegen_parse_block(
  CContext cx, Node* node, rcstr name
) {
  LLVMBasicBlockRef entry =
    LLVMAppendBasicBlockInContext(cx.gen.context, cx.func->value, name);
  LLVMPositionBuilderAtEnd(cx.gen.builder, entry);

  for (Node* branch = node->unary; branch != nullptr; branch = branch->next) {
    unused LLVMValueRef ret = codegen_parse(cx, branch);
  }
  return entry;
}

static LLVMValueRef codegen_parse(CContext cx, Node* node) {
  if (node->kind == ND_Operation) {
    return codegen_oper(cx, node);

  } else if (node->kind == ND_Negation) {
    LLVMValueRef value = codegen_parse(cx, node->unary);
    return LLVMBuildNeg(cx.gen.builder, value, "neg");

  } else if (node->kind == ND_Return) {
    LLVMValueRef val = codegen_parse(cx, node->unary);
    if (node->unary->kind == ND_Variable) {
      LLVMTypeRef val_type = codegen_type(cx, node->unary);
      val = LLVMBuildLoad2(cx.gen.builder, val_type, val, "");
    }
    return LLVMBuildRet(cx.gen.builder, val);

  } else if (node->kind == ND_Type) {
    eputs("Raw ND_Type unimplemented");
    exit(1);

  } else if (node->kind == ND_Decl) {
    LLVMTypeRef type = codegen_type(cx, node->declaration.type);
    LLVMValueRef decl =
      LLVMBuildAlloca(cx.gen.builder, type, node->declaration.name->array);
    LLVMValueRef val = codegen_parse(cx, node->declaration.value);
    LLVMBuildStore(cx.gen.builder, val, decl);

    DeclVar_vector_push(
      &cx.vars,
      (DeclVar){
        .variable = decl,
        .name = node->declaration.name->array,
      }
    );
    return decl;

  } else if (node->kind == ND_Value) {
    return codegen_value(cx, node);

  } else if (node->kind == ND_Variable) {
    DeclVar* decl_var = get_decl_var(cx.vars, node->unary->declaration.name);
    if (decl_var == nullptr) {
      eputs("ND_Variable not found");
      exit(1);
    }
    return decl_var->variable;

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
    LLVMTypeRef type = codegen_type(cx, node->unary);
    LLVMValueRef ptr = codegen_parse(cx, node->unary);
    LLVMValueRef load =
      LLVMBuildLoad2(cx.gen.builder, LLVMPointerType(type, 0), ptr, "");
    return load;

  } else if (node->kind == ND_Function) {
    eputs("Raw ND_Function unimplemented");
    exit(1);

  } else if (node->kind == ND_If) {
    LLVMValueRef cond = codegen_parse(cx, node->if_node.cond);
    LLVMBasicBlockRef then =
      codegen_parse_block(cx, node->if_node.then, "if_then");
    LLVMBasicBlockRef elseb =
      codegen_parse_block(cx, node->if_node.elseb, "if_elseb");
    LLVMValueRef if_block = LLVMBuildCondBr(cx.gen.builder, cond, then, elseb);
    return if_block;

  } else if (node->kind == ND_While) {
    eputs("ND_While unimplemented");
    exit(1);

  } else if (node->kind == ND_Call) {
    return codegen_call(cx, node);
  }
  print_cdgn_err(node->kind);
}

static LLVMValueRef codegen_oper(CContext cx, Node* node) {
  if (node->operation.kind == OP_Asg) {
    if (node->operation.lhs->kind != ND_Variable) {
      eputs("Variable required on left side for assignment");
      exit(1);
    }
    LLVMValueRef lhs = codegen_parse(cx, node->operation.lhs);
    LLVMValueRef rhs = codegen_parse(cx, node->operation.rhs);
    if (node->operation.rhs->kind == ND_Variable) {
      LLVMTypeRef rhs_type = codegen_type(cx, node->operation.rhs);
      rhs = LLVMBuildLoad2(cx.gen.builder, rhs_type, rhs, "");
    }
    return LLVMBuildStore(cx.gen.builder, rhs, lhs);
  }

  LLVMValueRef lhs = codegen_parse(cx, node->operation.lhs);
  if (node->operation.lhs->kind == ND_Variable) {
    LLVMTypeRef lhs_type = codegen_type(cx, node->operation.lhs);
    lhs = LLVMBuildLoad2(cx.gen.builder, lhs_type, lhs, "");
  }
  LLVMValueRef rhs = codegen_parse(cx, node->operation.rhs);
  if (node->operation.rhs->kind == ND_Variable) {
    LLVMTypeRef rhs_type = codegen_type(cx, node->operation.rhs);
    rhs = LLVMBuildLoad2(cx.gen.builder, rhs_type, rhs, "");
  }
  switch (node->operation.kind) {
    case OP_Add:
      return LLVMBuildAdd(cx.gen.builder, lhs, rhs, "add");
    case OP_Sub:
      return LLVMBuildSub(cx.gen.builder, lhs, rhs, "sub");
    case OP_Mul:
      return LLVMBuildMul(cx.gen.builder, lhs, rhs, "mul");
    case OP_Div:
      return LLVMBuildUDiv(cx.gen.builder, lhs, rhs, "div");
    case OP_Eq:
      return LLVMBuildICmp(cx.gen.builder, LLVMIntEQ, lhs, rhs, "eq");
    case OP_NEq:
      return LLVMBuildICmp(cx.gen.builder, LLVMIntNE, lhs, rhs, "neq");
    case OP_Lt:
      return LLVMBuildICmp(cx.gen.builder, LLVMIntSLT, lhs, rhs, "lt");
    case OP_Lte:
      return LLVMBuildICmp(cx.gen.builder, LLVMIntSLE, lhs, rhs, "lte");
    case OP_Gt:
      return LLVMBuildICmp(cx.gen.builder, LLVMIntSGT, lhs, rhs, "gt");
    case OP_Gte:
      return LLVMBuildICmp(cx.gen.builder, LLVMIntSGE, lhs, rhs, "gte");
    case OP_Asg:
      print_cdgn_err(node->kind);
    case OP_ArrIdx:
      return LLVMBuildInBoundsGEP2(
        cx.gen.builder, LLVMTypeOf(lhs), lhs, &rhs, 1, "arr_idx"
      );
  }
  print_cdgn_err(node->kind);
}

static LLVMValueRef codegen_value(CContext cx, Node* node) {
  if (is_integer(node->value.type) == true) {
    LLVMTypeRef type = codegen_type(cx, node->value.type);
    return LLVMConstInt(type, atoi(node->value.basic->array), true);

  } else if (node->value.type->type.kind == TP_Str) {
    LLVMTypeRef type = LLVMArrayType(
      LLVMInt8TypeInContext(cx.gen.context), node->value.basic->capacity + 1
    );
    LLVMValueRef str_val = LLVMConstStringInContext(
      cx.gen.context, node->value.basic->array, node->value.basic->capacity,
      false
    );
    LLVMValueRef global_str = LLVMAddGlobal(cx.gen.module, type, ".str");
    LLVMSetInitializer(global_str, str_val);
    LLVMSetLinkage(global_str, LLVMPrivateLinkage);
    LLVMSetUnnamedAddr(global_str, LLVMGlobalUnnamedAddr);
    LLVMSetAlignment(global_str, 1);
    LLVMSetGlobalConstant(global_str, true);
    return global_str;

  } else if (node->value.type->type.kind == TP_Arr) {
    // TODO: Implement array value
  }
  print_cdgn_err(node->kind);
}

static LLVMValueRef codegen_call(CContext cx, Node* node) {
  DeclFn* decl_fn = get_decl_fn(cx.funcs, node->call_node.name);
  if (decl_fn == nullptr) {
    eputs("ND_Call function not found");
    exit(1);
  }
  LLVMValueRefVector* call_args = LLVMValueRef_vector_make(2);
  for (Node* arg = node->call_node.args; arg != nullptr; arg = arg->next) {
    LLVMValueRef value = codegen_parse(cx, arg);
    if (arg->kind == ND_Variable || arg->kind == ND_Deref ||
        (arg->kind == ND_Operation && arg->operation.kind == OP_ArrIdx)) {
      value = LLVMBuildLoad2(cx.gen.builder, LLVMTypeOf(value), value, "");
    }
    LLVMValueRef_vector_push(&call_args, value);
  }
  LLVMValueRef result = LLVMBuildCall2(
    cx.gen.builder, decl_fn->type, decl_fn->value, call_args->buffer,
    call_args->length, decl_fn->name
  );
  free(call_args);
  return result;
}

static LLVMTypeRef codegen_type(CContext cx, Node* node) {
  if (node->kind == ND_Variable) {
    return codegen_type(cx, node->unary);

  } else if (node->kind == ND_Decl || node->kind == ND_ArgVar) {
    return codegen_type(cx, node->declaration.type);

  } else if (node->kind == ND_Value) {
    return codegen_type(cx, node->value.type);

  } else if (node->kind == ND_Type) {
    if (is_integer(node)) {
      return LLVMIntTypeInContext(cx.gen.context, node->type.bit_width);

    } else if (node->type.kind == TP_Flt) {
      if (node->type.bit_width == 15) {
        return LLVMBFloatTypeInContext(cx.gen.context);
      } else if (node->type.bit_width == 16) {
        return LLVMHalfTypeInContext(cx.gen.context);
      } else if (node->type.bit_width == 32) {
        return LLVMFloatTypeInContext(cx.gen.context);
      } else if (node->type.bit_width == 64) {
        return LLVMDoubleTypeInContext(cx.gen.context);
      } else if (node->type.bit_width == 128) {
        return LLVMFP128TypeInContext(cx.gen.context);
      }

    } else if (node->type.kind == TP_Ptr) {
      return LLVMPointerType(codegen_type(cx, node->type.base), 0);

    } else if (node->type.kind == TP_Arr) {
      return LLVMArrayType2(
        codegen_type(cx, node->type.array.base), node->type.array.size
      );
    }
    eputs("Type unimplemented");
    exit(1);
  }
  eputs("Node is not a type");
  exit(1);
}
