#include "Node.h"
#include <vector>

//------------------------------------------------------------------------------

struct TranslationUnit : public Node {
  TranslationUnit() : Node(NODE_TRANSLATION_UNIT, nullptr, nullptr) {}
};

//------------------------------------------------------------------------------

struct Declaration : public Node {
  Declaration() : Node(NODE_DECLARATION, nullptr, nullptr) {}

  Node* _decltype = nullptr;
  Node* _declname = nullptr;
  Node* _declop   = nullptr;
  Node* _declinit = nullptr;
};

//------------------------------------------------------------------------------

struct TemplateDeclaration : public Node {
  TemplateDeclaration() : Node(NODE_TEMPLATE_DECLARATION, nullptr, nullptr) {}

  Node* _keyword = nullptr;
  Node* _params  = nullptr;
  Node* _class   = nullptr;
};

//------------------------------------------------------------------------------

struct ParameterList : public Node {
  ParameterList() : Node(NODE_PARAMETER_LIST, nullptr, nullptr) {}
};

struct TemplateParameterList : public Node {
  TemplateParameterList() : Node(NODE_TEMPLATE_PARAMETER_LIST, nullptr, nullptr) {}
};

struct ClassSpecifier : public Node {
  ClassSpecifier() : Node(NODE_CLASS_SPECIFIER, nullptr, nullptr) {}
};

struct FieldDeclarationList : public Node {
  FieldDeclarationList() : Node(NODE_FIELD_DECLARATION_LIST, nullptr, nullptr) {}
};

struct CompoundStatement : public Node {
  CompoundStatement() : Node(NODE_COMPOUND_STATEMENT, nullptr, nullptr) {}
};

struct Constructor : public Node {
  Constructor() : Node(NODE_CONSTRUCTOR, nullptr, nullptr) {}

  Node* _decl;
  Node* _params;
  Node* _body;
};
