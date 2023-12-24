#include <parser/mod.h>
#include <stdio.h>
#include <utility/mod.h>

static i32 indent = 0;
static void print_indent() {
  for (i32 i = 0; i < indent; ++i) {
    eputc('|');
    eputc(' ');
  }
  indent += 1;
}

static void print_branch(Node* node) {
  print_indent();

  if (node->kind == ND_None) {
    eputs("None");

  } else if (node->kind == ND_Operation) {
    switch (node->operation.kind) { // clang-format off
      case OP_Add:  eputs("Operation: Add");  break;
      case OP_Sub:  eputs("Operation: Sub");  break;
      case OP_Mul:  eputs("Operation: Mul");  break;
      case OP_Div:  eputs("Operation: Div");  break;
      case OP_Eq:   eputs("Operation: Eq");   break;
      case OP_NEq:  eputs("Operation: Not");  break;
      case OP_Lt:   eputs("Operation: Lt");   break;
      case OP_Lte:  eputs("Operation: Lte");  break;
      case OP_Gte:  eputs("Operation: Gte");  break;
      case OP_Gt:   eputs("Operation: Gt");   break;
      case OP_Asg:  eputs("Operation: Asg");  break;
      case OP_Decl: eputs("Operation: Decl"); break;
    } // clang-format on
    print_branch(node->operation.lhs);
    print_branch(node->operation.rhs);

  } else if (node->kind == ND_Negation) {
    eputs("Negation");
    print_branch(node->unary);

  } else if (node->kind == ND_ExprStmt) {
    eputs("ExprStmt");
    print_branch(node->unary);

  } else if (node->kind == ND_Return) {
    eputs("Return");
    print_branch(node->unary);

  } else if (node->kind == ND_Block) {
    eputs("Block:");
    for (Node* branch = node->unary; branch != nullptr; branch = branch->next) {
      print_branch(branch);
    }
  } else if (node->kind == ND_Variable) {
    eprintf("Variable = %s\n", node->variable->name.array);

  } else if (node->kind == ND_Value) {
    eprintf("Value = %li\n", node->value.i_num);
    // if (node->value.kind == TP_Int) {
    //   eprintf("Value: TP_Int = %li\n", node->value.i_num);
    // } else {
    //   eputs("unimplemented");
    // }
  } else if (node->kind == ND_If) {
    eputs("If:");
    print_branch(node->if_node.cond);
    print_branch(node->if_node.then);
    if (node->if_node.elseb) {
      print_branch(node->if_node.elseb);
    }
  } else if (node->kind == ND_While) {
    eputs("While:");
    print_branch(node->while_node.cond);
    print_branch(node->while_node.then);

  } else if (node->kind == ND_Call) {
    eprintf("Call = %s\n", node->call_node.name.array);
    for (Node* branch = node->call_node.args; branch != nullptr;
         branch = branch->next) {
      print_branch(branch);
    }
  }
  indent -= 1;
}

void print_ast(Function* prog) {
  for (Function* branch = prog; branch != nullptr; branch = branch->next) {
    if (branch != prog) {
      eputs("--------------------------------------");
    }
    indent += 1;
    eprintf("Function = %s\n", branch->name.array);
    for (Node* arg = branch->args; arg != nullptr; arg = arg->next) {
      print_branch(arg);
    }
    indent -= 1;
    print_branch(branch->body);
  }
}
