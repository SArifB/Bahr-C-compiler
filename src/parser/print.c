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
    eputs("ND_None");

  } else if (node->kind == ND_Operation) {
    switch (node->operation.kind) { // clang-format off
      case OP_Add:  eputs("ND_Operation: OP_Add");  break;
      case OP_Sub:  eputs("ND_Operation: OP_Sub");  break;
      case OP_Mul:  eputs("ND_Operation: OP_Mul");  break;
      case OP_Div:  eputs("ND_Operation: OP_Div");  break;
      case OP_Eq:   eputs("ND_Operation: OP_Eq");   break;
      case OP_NEq:  eputs("ND_Operation: OP_Not");  break;
      case OP_Lt:   eputs("ND_Operation: OP_Lt");   break;
      case OP_Lte:  eputs("ND_Operation: OP_Lte");  break;
      case OP_Gte:  eputs("ND_Operation: OP_Gte");  break;
      case OP_Gt:   eputs("ND_Operation: OP_Gt");   break;
      case OP_Asg:  eputs("ND_Operation: OP_Asg");  break;
    } // clang-format on
    print_branch(node->operation.lhs);
    print_branch(node->operation.rhs);

  } else if (node->kind == ND_Negation) {
    eputs("ND_Negation");
    print_branch(node->unary);

  } else if (node->kind == ND_ExprStmt) {
    eputs("ND_ExprStmt");
    print_branch(node->unary);

  } else if (node->kind == ND_Return) {
    eputs("ND_Return");
    print_branch(node->unary);

  } else if (node->kind == ND_Block) {
    eputs("ND_Block:");
    for (Node* branch = node->unary; branch != nullptr; branch = branch->next) {
      print_branch(branch);
    }
  } else if (node->kind == ND_Variable) {
    eprintf("ND_Variable: %s\n", node->variable->name.array);

  } else if (node->kind == ND_Value) {
    eprintf("ND_Value: TP_Int = %li\n", node->value.i_num);
    // if (node->value.kind == TP_Int) {
    //   eprintf("ND_Value: TP_Int = %li\n", node->value.i_num);
    // } else {
    //   eputs("unimplemented");
    // }
  } else if (node->kind == ND_If) {
    eputs("ND_If: Cond");
    print_branch(node->if_node.cond);

    indent -= 1;
    print_indent();
    eputs("ND_If: Then");
    print_branch(node->if_node.then);

    if (node->if_node.elseb) {
      indent -= 1;
      print_indent();
      eputs("ND_If: Else");
      print_branch(node->if_node.elseb);
    }
  } else if (node->kind == ND_While) {
    eputs("ND_While: Cond");
    print_branch(node->while_node.cond);

    indent -= 1;
    print_indent();
    eputs("ND_While: Then");
    print_branch(node->while_node.then);

  } else if (node->kind == ND_Call) {
    eprintf("ND_Call: %s\n", node->call_node.name.array);
    for (Node* branch = node->call_node.args; branch != nullptr;
         branch = branch->next) {
      print_branch(branch);
    }
  }
  indent -= 1;
}

void print_ast(Function* prog) {
  print_branch(prog->body);
}
