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

struct Constructor : public Node {
  Constructor(Node* _specs, Node* _name, Node* _params, Node* _body)
  : Node(NODE_CONSTRUCTOR, nullptr, nullptr) {
    append(_specs);
    append(_name);
    append(_params);
    append(_body);
    this->_specs  = _specs;
    this->_name   = _name;
    this->_params = _params;
    this->_body   = _body;
  }

  Node* _specs;
  Node* _name;
  Node* _params;
  Node* _body;
};

//------------------------------------------------------------------------------

struct Declaration : public Node {
  Declaration(Node* _specs, Node* _decltype, Node* _declname, Node* _declop, Node* _declinit, Node* _semi)
  : Node(NODE_DECLARATION, nullptr, nullptr) {
    append(_specs);
    append(_decltype);
    append(_declname);
    append(_declop);
    append(_declinit);
    append(_semi);
    this->_specs    = _specs;
    this->_decltype = _decltype;
    this->_declname = _declname;
    this->_declop   = _declop;
    this->_declinit = _declinit;
    this->_semi     = _semi;
  }

  Declaration(Node* _specs, Node* _decltype, Node* _declname, Node* _semi)
  : Node(NODE_DECLARATION, nullptr, nullptr) {
    append(_specs);
    append(_decltype);
    append(_declname);
    append(_semi);
    this->_specs    = _specs;
    this->_decltype = _decltype;
    this->_declname = _declname;
    this->_semi     = _semi;
  }

  Node* _specs    = nullptr;
  Node* _decltype = nullptr;
  Node* _declname = nullptr;
  Node* _declop   = nullptr;
  Node* _declinit = nullptr;
  Node* _semi     = nullptr;
};

//------------------------------------------------------------------------------

struct FieldDeclarationList : public Node {
  FieldDeclarationList() : Node(NODE_FIELD_DECLARATION_LIST, nullptr, nullptr) {}

  Node* ldelim = nullptr;
  std::vector<Node*> decls;
  Node* rdelim = nullptr;
};

//------------------------------------------------------------------------------

struct ParameterList : public Node {
  ParameterList() : Node(NODE_PARAMETER_LIST, nullptr, nullptr) {}

  Node* ldelim = nullptr;
  std::vector<Node*> decls;
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
