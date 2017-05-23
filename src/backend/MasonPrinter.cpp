#include "MasonPrinter.hpp"

namespace OpenABL {

void MasonPrinter::print(const AST::MemberInitEntry &) {}
void MasonPrinter::print(const AST::AgentCreationExpression &) {}
void MasonPrinter::print(const AST::NewArrayExpression &) {}
void MasonPrinter::print(const AST::ExpressionStatement &) {}
void MasonPrinter::print(const AST::SimulateStatement &) {}
void MasonPrinter::print(const AST::ArrayType &) {}
void MasonPrinter::print(const AST::Param &) {}

void MasonPrinter::print(const AST::SimpleType &type) {
  switch (type.resolved.getTypeId()) {
    case Type::BOOL:
    case Type::INT32:
      *this << type.resolved;
      return;
    case Type::FLOAT32:
      // TODO Use doubles, because that's what Mason uses.
      // Need to figure out what semantics we want to have here.
      *this << "double";
      return;
    case Type::STRING:
      *this << "String";
      return;
    case Type::VEC2:
      *this << "Double2D";
      return;
    case Type::VEC3:
      *this << "Double3D";
      return;
    default:
      assert(0); // TODO
  }
}

void MasonPrinter::print(const AST::VarExpression &expr) {
  const ScopeEntry &entry = script.scope.get(expr.var->id);
  if (entry.isGlobal) {
    *this << "Sim." << *expr.var;
  } else {
    *this << *expr.var;
  }
}

void MasonPrinter::print(const AST::MemberAccessExpression &expr) {
  if (expr.expr->type.isAgent()) {
    // TODO Right now this simply refers to "this", which is obviously
    // incorrect. Correct mapping of the in/out variables, near agents etc
    // needs to be implemented here
    *this << "this." << expr.member;
  } else {
    GenericPrinter::print(expr);
  }
}

static void printBinaryOp(MasonPrinter &p, AST::BinaryOp op,
                          const AST::Expression &left, const AST::Expression &right) {
  Type l = left.type;
  Type r = right.type;
  if (l.isVec() || r.isVec()) {
    p << left << ".";
    switch (op) {
      case AST::BinaryOp::ADD: p << "add"; break;
      case AST::BinaryOp::SUB: p << "subtract"; break;
      case AST::BinaryOp::MUL: p << "multiply"; break;
      case AST::BinaryOp::DIV:
        // Emulate divide via multiply by reciprocal
        p << "multiply(1. / " << right << ")";
        return;
      default:
        assert(0);
    }
    p << "(" << right << ")";
    return;
  }

  p << "(" << left << " " << AST::getBinaryOpSigil(op) << " " << right << ")";
}

void MasonPrinter::print(const AST::BinaryOpExpression &expr) {
  printBinaryOp(*this, expr.op, *expr.left, * expr.right);
}

static void printTypeCtor(MasonPrinter &p, const AST::CallExpression &expr) {
  Type t = expr.type;
  if (t.isVec()) {
    int vecWidth = t.getTypeId() == Type::VEC2 ? 2 : 3;
    p << "new Double" << vecWidth << "D(";
    size_t numArgs = expr.args->size();
    if (numArgs == 1) {
      // TODO Multiple evaluation
      const AST::Expression &arg = *expr.getArg(0).expr;
      p << arg << ", " << arg;
      if (vecWidth == 3) {
        p << ", " << arg;
      }
    } else {
      p.printArgs(expr);
    }
    p << ")";
  } else {
    p << "(" << t << ") " << *expr.getArg(0).expr;
  }
}
static void printBuiltin(MasonPrinter &p, const AST::CallExpression &expr) {
  if (expr.name == "dist") {
    p << expr.getArg(0) << ".distance(" << expr.getArg(1) << ")";
  } else {
    // TODO Handle other builtins
    const FunctionSignature &sig = expr.calledSig;
    p << sig.name << "(";
    p.printArgs(expr);
    p << ")";
  }
}
void MasonPrinter::print(const AST::CallExpression &expr) {
  if (expr.isCtor()) {
    printTypeCtor(*this, expr);
  } else if (expr.isBuiltin()) {
    printBuiltin(*this, expr);
  } else {
    *this << "Sim." << expr.name << "(";
    this->printArgs(expr);
    *this << ")";
  }
}

void MasonPrinter::print(const AST::AssignOpStatement &stmt) {
  *this << *stmt.left << " = ";
  printBinaryOp(*this, stmt.op, *stmt.left, *stmt.right);
  *this << ";";
}

void MasonPrinter::print(const AST::UnaryOpExpression &expr) {
  Type t = expr.expr->type;
  if (t.isVec()) {
    if (expr.op == AST::UnaryOp::PLUS) {
      // Nothing to do
      *this << *expr.expr;
    } else if (expr.op == AST::UnaryOp::MINUS) {
      *this << *expr.expr << ".negate()";
    } else {
      assert(0);
    }
    return;
  }

  GenericPrinter::print(expr);
}

void MasonPrinter::print(const AST::VarDeclarationStatement &stmt) {
  *this << *stmt.type << " " << *stmt.var;
  if (stmt.initializer) {
    *this << " = " << *stmt.initializer;
  }
  *this << ";";
}

void MasonPrinter::print(const AST::ForStatement &stmt) {
  if (stmt.isNear()) {
    // TODO Nearest neighbor loop
    AST::AgentDeclaration *agentDecl = stmt.type->resolved.getAgentDecl();
    std::string iLabel = makeAnonLabel();
    *this << "Bag _bag = _sim.env.getAllObjects();" << nl
          << "for (int " << iLabel << " = 0; " << iLabel << " < _bag.size(); "
          << iLabel << "++) {" << indent << nl
          << agentDecl->name << " " << *stmt.var << " = "
          << "(" << agentDecl->name << ") _bag.get(" << iLabel << ");" << nl
          << *stmt.stmt
          << outdent << nl << "}";
  } else if (stmt.isRange()) {
    // TODO Range loop
  } else {
    // TODO Ordinary collection loop
  }
}

void MasonPrinter::print(const AST::ConstDeclaration &decl) {
  *this << "public static " << *decl.type << " " << *decl.var
        << " = " << *decl.expr << ";";
}

void MasonPrinter::print(const AST::FunctionDeclaration &decl) {
  if (decl.isStep) {
    *this << "public void " << decl.name << "(SimState state) {" << indent << nl
          << "Sim _sim = (Sim) state;"
          << *decl.stmts
          << outdent << nl << "}";
    // TODO
  } else {
    assert(0); // TODO
  }
}

void MasonPrinter::print(const AST::AgentMember &member) {
  *this << *member.type << " " << member.name << ";";
}

void MasonPrinter::print(const AST::AgentDeclaration &decl) {
  *this << "import sim.engine.*;" << nl
        << "import sim.util.*;" << nl << nl
        << "public class " << decl.name << " {" << indent
        << *decl.members << nl << nl;

  // Print constructor
  *this << "public " << decl.name << "(";
  bool first = true;
  for (AST::AgentMemberPtr &member : *decl.members) {
    if (!first) *this << ", ";
    first = false;
    *this << *member->type << " " << member->name;
  }
  *this << ") {" << indent;
  for (AST::AgentMemberPtr &member : *decl.members) {
    *this << nl << "this." << member->name << " = " << member->name << ";";
  }
  *this << outdent << nl << "}";

  for (AST::FunctionDeclaration *fn : script.funcs) {
    if (fn->isStep && &fn->stepAgent() == &decl) {
      *this << nl << nl << *fn;
    }
  }

  *this << outdent << nl << "}";
}

void MasonPrinter::print(const AST::Script &script) {
  *this << "import sim.engine.*;" << nl
        << "import sim.util.*;" << nl
        << "import sim.field.continuous.*;" << nl << nl
        << "public class Sim extends SimState {" << indent << nl;

  for (const AST::ConstDeclaration *decl : script.consts) {
    *this << *decl << nl;
  }

  // TODO Replace dummy bounds
  *this << "public Continuous2D env = new Continuous2D(1.0, 100, 100);" << nl << nl
        << "public Sim(long seed) {" << indent << nl
        << "super(seed);"
        << outdent << nl << "}" << nl << nl
        << "public void start() {" << indent << nl
        << "super.start();"
        // TODO Agent initialization here
        << outdent << nl << "}" << nl
        << "public static void main(String[] args) {" << indent << nl
        << "doLoop(Sim.class, args);" << nl
        << "System.exit(0);"
        << outdent << nl << "}"
        << outdent << nl << "}";
}

}