#include <parser/ctors.h>
#include <stdio.h>
#include <utility/mod.h>

static void print_indent(i32* indent) {
  for (i32 i = 0; i < *indent; ++i) {
    eputc('|');
    eputc(' ');
  }
  *indent += 1;
}

static void print_branch(Node* node, i32* indent) {
  print_indent(indent);

  if (node->kind == ND_None) {
    eputs("None");

  } else if (node->kind == ND_Operation) {
    switch (node->operation.kind) {  // clang-format off
      case OP_Add:    eputs("Operation: Add");    break;
      case OP_Sub:    eputs("Operation: Sub");    break;
      case OP_Mul:    eputs("Operation: Mul");    break;
      case OP_Div:    eputs("Operation: Div");    break;
      case OP_Eq:     eputs("Operation: Eq");     break;
      case OP_NEq:    eputs("Operation: Not");    break;
      case OP_Lt:     eputs("Operation: Lt");     break;
      case OP_Lte:    eputs("Operation: Lte");    break;
      case OP_Gte:    eputs("Operation: Gte");    break;
      case OP_Gt:     eputs("Operation: Gt");     break;
      case OP_Asg:    eputs("Operation: Asg");    break;
      case OP_ArrIdx: eputs("Operation: ArrIdx"); break;
    }  // clang-format on
    print_branch(node->operation.lhs, indent);
    print_branch(node->operation.rhs, indent);

  } else if (node->kind == ND_Negation) {
    eputs("Negation");
    print_branch(node->unary, indent);

  } else if (node->kind == ND_Return) {
    eputs("Return");
    print_branch(node->unary, indent);

  } else if (node->kind == ND_Block) {
    eputs("Block:");
    for (Node* branch = node->unary; branch != nullptr; branch = branch->next) {
      print_branch(branch, indent);
    }

  } else if (node->kind == ND_Decl) {
    eprintf("Declaration = %s\n", node->declaration.name->array);
    print_branch(node->declaration.type, indent);
    if (node->declaration.value != nullptr) {
      print_branch(node->declaration.value, indent);
    } else {
      print_indent(indent);
      eputs("Value = Undefined");
      *indent -= 1;
    }

  } else if (node->kind == ND_Type) {
    switch (node->type.kind) {  // clang-format off
      case TP_Undf: eputs("Type: Undf"); break;
      case TP_Unit: eputs("Type: Unit"); break;
      case TP_SInt: eputs("Type: SInt"); break;
      case TP_UInt: eputs("Type: UInt"); break;
      case TP_Flt:  eputs("Type: Flt");  break;
      case TP_Ptr:  eputs("Type: Ptr");  break;
      case TP_Arr:  eputs("Type: Arr");  break;
      case TP_Str:  eputs("Type: Str");  break;
    }                                 // clang-format on
    if (node->type.kind == TP_Flt ||  //
        node->type.kind == TP_SInt || node->type.kind == TP_UInt) {
      print_indent(indent);
      eprintf("Width = %zu\n", node->type.bit_width);
      *indent -= 1;
    } else if (node->type.kind == TP_Ptr) {
      print_branch(node->type.base, indent);
    } else if (node->type.kind == TP_Arr) {
      print_branch(node->type.array.base, indent);
      print_indent(indent);
      eprintf("Size = %zu\n", node->type.array.size);
      *indent -= 1;
    }

  } else if (node->kind == ND_Value) {
    if (node->value.type->type.kind == TP_Ptr) {
      eputs("Value = Ptr");
      print_branch(node->value.base, indent);
    } else if (node->value.type->type.kind == TP_Arr) {
      eputs("Value = Arr");
      for (Node* val = node->value.base; val != nullptr; val = val->next) {
        print_branch(val, indent);
      }
    } else {
      eprintf("Value = %s\n", node->value.basic->array);
    }

  } else if (node->kind == ND_Variable) {
    eprintf("Variable = %s\n", node->unary->declaration.name->array);
    print_branch(node->unary->declaration.type, indent);

  } else if (node->kind == ND_ArgVar) {
    eprintf("Argument = %s\n", node->declaration.name->array);
    print_branch(node->declaration.type, indent);

  } else if (node->kind == ND_If) {
    eputs("If:");
    print_branch(node->if_node.cond, indent);
    print_branch(node->if_node.then, indent);
    if (node->if_node.elseb != nullptr) {
      print_branch(node->if_node.elseb, indent);
    }
  } else if (node->kind == ND_While) {
    eputs("While:");
    print_branch(node->while_node.cond, indent);
    print_branch(node->while_node.then, indent);

  } else if (node->kind == ND_Call) {
    eprintf("Call = %s\n", node->call_node.name->array);
    for (Node* branch = node->call_node.args; branch != nullptr;
         branch = branch->next) {
      print_branch(branch, indent);
    }
  }
  *indent -= 1;
}

void print_ast(Node* prog) {
  for (Node* branch = prog; branch != nullptr; branch = branch->next) {
    if (branch != prog) {
      eputs("--------------------------------------");
    }
    i32 indent = 0;
    eprintf("Function = %s\n", branch->function.name->array);
    indent += 1;
    print_branch(branch->function.ret_type, &indent);
    for (Node* arg = branch->function.args; arg != nullptr; arg = arg->next) {
      print_branch(arg, &indent);
    }
    indent -= 1;
    if (branch->function.body != nullptr) {
      print_branch(branch->function.body, &indent);
    }
  }
}
