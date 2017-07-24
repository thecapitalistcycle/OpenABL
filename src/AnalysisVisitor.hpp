#pragma once

#include <map>
#include <stack>
#include "ASTVisitor.hpp"
#include "Analysis.hpp"
#include "ErrorHandling.hpp"

namespace OpenABL {

struct AnalysisVisitor : public AST::Visitor {
  AnalysisVisitor(
      AST::Script &script, const std::map<std::string, std::string> &params,
      ErrorStream &err, BuiltinFunctions &builtins
  ) : script(script), params(params), builtins(builtins), err(err), scope(script.scope) {}

  void enter(AST::Var &);
  void enter(AST::Literal &);
  void enter(AST::VarExpression &);
  void enter(AST::UnaryOpExpression &);
  void enter(AST::BinaryOpExpression &);
  void enter(AST::CallExpression &);
  void enter(AST::MemberAccessExpression &);
  void enter(AST::ArrayAccessExpression &);
  void enter(AST::TernaryExpression &);
  void enter(AST::MemberInitEntry &);
  void enter(AST::AgentCreationExpression &);
  void enter(AST::ArrayInitExpression &);
  void enter(AST::NewArrayExpression &);
  void enter(AST::ExpressionStatement &);
  void enter(AST::AssignStatement &);
  void enter(AST::AssignOpStatement &);
  void enter(AST::BlockStatement &);
  void enter(AST::VarDeclarationStatement &);
  void enter(AST::IfStatement &);
  void enter(AST::WhileStatement &);
  void enter(AST::ForStatement &);
  void enter(AST::SimulateStatement &);
  void enter(AST::ReturnStatement &);
  void enter(AST::SimpleType &);
  void enter(AST::Param &);
  void enter(AST::FunctionDeclaration &);
  void enter(AST::AgentMember &);
  void enter(AST::AgentDeclaration &);
  void enter(AST::ConstDeclaration &);
  void enter(AST::EnvironmentDeclaration &);
  void enter(AST::Script &);
  void leave(AST::Var &);
  void leave(AST::Literal &);
  void leave(AST::VarExpression &);
  void leave(AST::UnaryOpExpression &);
  void leave(AST::BinaryOpExpression &);
  void leave(AST::CallExpression &);
  void leave(AST::MemberAccessExpression &);
  void leave(AST::ArrayAccessExpression &);
  void leave(AST::TernaryExpression &);
  void leave(AST::MemberInitEntry &);
  void leave(AST::AgentCreationExpression &);
  void leave(AST::ArrayInitExpression &);
  void leave(AST::NewArrayExpression &);
  void leave(AST::ExpressionStatement &);
  void leave(AST::AssignStatement &);
  void leave(AST::AssignOpStatement &);
  void leave(AST::BlockStatement &);
  void leave(AST::VarDeclarationStatement &);
  void leave(AST::IfStatement &);
  void leave(AST::WhileStatement &);
  void leave(AST::ForStatement &);
  void leave(AST::SimulateStatement &);
  void leave(AST::ReturnStatement &);
  void leave(AST::SimpleType &);
  void leave(AST::Param &);
  void leave(AST::FunctionDeclaration &);
  void leave(AST::AgentMember &);
  void leave(AST::AgentDeclaration &);
  void leave(AST::ConstDeclaration &);
  void leave(AST::EnvironmentDeclaration &);
  void leave(AST::Script &);

private:
  void declareVar(AST::Var &, Type, bool isConst, bool isGlobal, Value val);
  void pushVarScope();
  void popVarScope();
  Type resolveAstType(AST::Type &);
  Value evalExpression(const AST::Expression &expr);

  using VarMap = std::map<std::string, VarId>;

  // Analyzed script
  AST::Script &script;
  // Simulation parameters
  const std::map<std::string, std::string> &params;
  // Builtin functions
  BuiltinFunctions &builtins;
  // Stream for error reporting
  ErrorStream &err;

  // Currently *visible* variables
  VarMap varMap;
  // Stack of previous visible variable scopes
  // This is inefficient, but we don't actually care...
  std::stack<VarMap> varMapStack;
  // Information about *all* variables, indexed by unique VarId's
  Scope &scope;
  // Current function
  AST::FunctionDeclaration *currentFunc;
  // Declared agents by name
  std::map<std::string, AST::AgentDeclaration *> agents;
  // Declared functions by name
  std::map<std::string, AST::FunctionDeclaration *> funcs;
  // Variable on which member accesses should be collected
  // (for FunctionDeclaration::accessedMembers)
  VarId collectAccessVar;
  // Radiuses used in near() loops
  std::vector<Value> radiuses;
};

}
