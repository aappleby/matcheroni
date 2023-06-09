#include "Node.h"
#include <vector>
#include "Matcheroni.h"

//------------------------------------------------------------------------------

struct ClassDeclaration : public Node {
  ClassDeclaration(Node* _name)
  : Node(NODE_CLASS_DECLARATION, nullptr, nullptr) {
    append(_name);
    this->_name    = _name;
  }

  Node* _name;
};

struct ClassDefinition : public Node {
  ClassDefinition(Node* _name, Node* _decls = nullptr)
  : Node(NODE_CLASS_DEFINITION, nullptr, nullptr) {
    append(_name);
    append(_decls);
    this->_name    = _name;
    this->_decls   = _decls;
  }

  Node* _name;
  Node* _decls;
};

//------------------------------------------------------------------------------

struct StructDeclaration : public Node {
  StructDeclaration(const Token* tok_a, const Token* tok_b)
  : Node(NODE_STRUCT_DECLARATION, tok_a, tok_b) {
  }
};

struct StructDefinition : public Node {
  StructDefinition(const Token* tok_a, const Token* tok_b)
  : Node(NODE_STRUCT_DEFINITION, tok_a, tok_b) {
  }
};

//------------------------------------------------------------------------------

struct CompoundStatement : public Node {
  CompoundStatement() : Node(NODE_COMPOUND_STATEMENT, nullptr, nullptr) {}

  std::vector<Node*> statements;
};

//------------------------------------------------------------------------------

struct NodeList : public Node {
  NodeList(NodeType type) : Node(type, nullptr, nullptr) {}

  std::vector<Node*> items;
};

//------------------------------------------------------------------------------

struct TemplateDeclaration : public Node {
  TemplateDeclaration() : Node(NODE_TEMPLATE_DECLARATION, nullptr, nullptr) {}

  Node* _keyword = nullptr;
  Node* _params  = nullptr;
  Node* _class   = nullptr;
};

//------------------------------------------------------------------------------

struct TemplateParameterList : public Node {
  TemplateParameterList() : Node(NODE_TEMPLATE_PARAMETER_LIST, nullptr, nullptr) {}
};

//------------------------------------------------------------------------------

struct TranslationUnit : public Node {
  TranslationUnit() : Node(NODE_TRANSLATION_UNIT, nullptr, nullptr) {}
};

//------------------------------------------------------------------------------
