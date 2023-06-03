#include "Node.h"
#include <vector>

//------------------------------------------------------------------------------

struct ClassDeclaration : public Node {
  ClassDeclaration(Node* _keyword, Node* _name, Node* _semi)
  : Node(NODE_CLASS_DECLARATION, nullptr, nullptr) {
    append(_keyword);
    append(_name);
    append(_semi);
    this->_keyword = _keyword;
    this->_name    = _name;
    this->_semi    = _semi;
  }

  Node* _keyword;
  Node* _name;
  Node* _semi;
};

//------------------------------------------------------------------------------
// Clang's CXXRecordDecl

struct ClassDefinition : public Node {
  ClassDefinition(Node* _keyword, Node* _name, Node* _decls, Node* _semi)
  : Node(NODE_CLASS_DEFINITION, nullptr, nullptr) {
    append(_keyword);
    append(_name);
    append(_decls);
    append(_semi);
    this->_keyword = _keyword;
    this->_name    = _name;
    this->_decls   = _decls;
    this->_semi    = _semi;
  }

  Node* _keyword;
  Node* _name;
  Node* _decls;
  Node* _semi;
};

//------------------------------------------------------------------------------

struct CompoundStatement : public Node {
  CompoundStatement() : Node(NODE_COMPOUND_STATEMENT, nullptr, nullptr) {}

  Node* ldelim = nullptr;
  std::vector<Node*> statements;
  Node* rdelim = nullptr;
};

//------------------------------------------------------------------------------

struct NodeList : public Node {
  NodeList(NodeType type) : Node(type, nullptr, nullptr) {}

  Node* ldelim = nullptr;
  std::vector<Node*> items;
  Node* rdelim = nullptr;
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
