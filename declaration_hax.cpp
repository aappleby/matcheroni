#include "Matcheroni.h"
#include "Node.h"

const Token* parse_expression (const Token* a, const Token* b);

//------------------------------------------------------------------------------

struct abstractDeclarator;
struct declarationSpecifiers;

const Token* parse_abstract_declarator       (const Token* a, const Token* b);
const Token* parse_declarator                (const Token* a, const Token* b);
const Token* parse_type_specifier            (const Token* a, const Token* b);
const Token* parse_initializer               (const Token* a, const Token* b);
const Token* parse_statement_compound        (const Token* a, const Token* b);

//------------------------------------------------------------------------------

struct functionSpecifier : public NodeMaker<functionSpecifier> {
  using pattern = Oneof<
    Keyword<"inline">,
    Keyword<"_Noreturn">,
    Keyword<"__inline__">,
    Keyword<"__stdcall">,
    Seq<Keyword<"__declspec">, Atom<'('>, NodeIdentifier, Atom<')'>>
  >;
};

//------------------------------------------------------------------------------

struct typeQualifier : public NodeMaker<typeQualifier> {
  using pattern = Oneof<
    Keyword<"const">,
    Keyword<"restrict">,
    Keyword<"volatile">,
    Keyword<"_Atomic">
  >;
};

//------------------------------------------------------------------------------

struct pointer : public NodeMaker<pointer> {
  using pattern =
  Seq<
    NodeOperator<"*">,
    Any<Oneof<
      NodeOperator<"*">,
      NodeKeyword<"const">,
      NodeKeyword<"restrict">,
      NodeKeyword<"volatile">,
      NodeKeyword<"_Atomic">
    >>
  >;
};

//------------------------------------------------------------------------------

struct parameterDeclaration : public NodeBase {
  using pattern = Oneof<
    Seq<
      declarationSpecifiers,
      Opt<Oneof<
        Ref<parse_declarator>,
        abstractDeclarator
      >>
    >,
    NodeOperator<"...">
  >;
};

//------------------------------------------------------------------------------

struct parameterTypeList : public NodeMaker<parameterTypeList> {
  using pattern = comma_separated<
    NodeMaker<parameterDeclaration>
  >;
};

//------------------------------------------------------------------------------

using arraySuffix = Oneof<
  Seq<Atom<'['>, Any<typeQualifier>, Opt<Ref<parse_expression>>, Atom<']'>>,
  Seq<Atom<'['>, Keyword<"static">, Any<typeQualifier>, Ref<parse_expression>, Atom<']'>>,
  Seq<Atom<'['>, Some<typeQualifier>, Keyword<"static">, Ref<parse_expression>, Atom<']'>>,
  Seq<Atom<'['>, Any<typeQualifier>, Atom<'*'>, Atom<']'>>
>;

//------------------------------------------------------------------------------

struct specifierQualifierList : public NodeMaker<specifierQualifierList> {
  using pattern = Some<Oneof<
    Ref<parse_type_specifier>,
    NodeKeyword<"const">,
    NodeKeyword<"restrict">,
    NodeKeyword<"volatile">,
    NodeKeyword<"_Atomic">
  >>;
};

//------------------------------------------------------------------------------

struct typeName : public NodeMaker<typeName> {
  using pattern = Seq<
    specifierQualifierList,
    Opt<Ref<parse_abstract_declarator>>
  >;
};

//------------------------------------------------------------------------------

using parameterList  = Seq<Atom<'('>, parameterTypeList, Atom<')'>>;
using identifierList = Seq<Atom<'('>, comma_separated<NodeIdentifier>, Atom<')'>>;
using emptyList      = Seq<Atom<'('>, Atom<')'>>;

//------------------------------------------------------------------------------

using bit_suffix = Seq<Atom<':'>, NodeConstant>;

//------------------------------------------------------------------------------

struct abstractDeclarator : public NodeMaker<abstractDeclarator> {
  using pattern = Oneof<
    pointer,
    Seq<
      Opt<pointer>,
      Seq<Atom<'('>, Ref<parse_abstract_declarator>, Atom<')'>>
    >,
    Seq<
      Opt<pointer>,
      Some<Oneof<
        arraySuffix,
        parameterList,
        emptyList
      >>
    >
  >;
};

//------------------------------------------------------------------------------

struct declarator : public NodeMaker<declarator> {
  using pattern = Seq<
    Oneof<
      Seq<Opt<pointer>, NodeIdentifier, Opt<bit_suffix>>,
      Seq<Opt<pointer>, Atom<'('>, Ref<parse_declarator>, Atom<')'>>
    >,
    Any<Oneof<
      arraySuffix,
      parameterList,
      identifierList,
      emptyList
    >>
  >;
};
const Token* parse_declarator(const Token* a, const Token* b) { return declarator::match(a, b); }

//------------------------------------------------------------------------------

struct fieldDeclarator : public NodeMaker<fieldDeclarator> {
  using pattern = Seq< declarator, Opt<bit_suffix> >;
};

//------------------------------------------------------------------------------

struct fieldDeclaration : public NodeMaker<fieldDeclaration> {
  using pattern = Seq<
    specifierQualifierList,
    Opt<comma_separated<
      fieldDeclarator
    >>,
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

struct fieldDeclarationList : public NodeMaker<fieldDeclarationList> {
  using pattern = Seq<
    Atom<'{'>,
    Some<fieldDeclaration>,
    Atom<'}'>
  >;
};

//------------------------------------------------------------------------------

struct structSpecifier : public NodeMaker<structSpecifier> {
  using pattern = Seq<
    Keyword<"struct">,
    Opt<LogTypename<NodeIdentifier>>,
    Opt<fieldDeclarationList>
  >;
};

//------------------------------------------------------------------------------

struct unionSpecifier : public NodeMaker<unionSpecifier> {
  using pattern = Seq<
    Keyword<"union">,
    Opt<LogTypename<NodeIdentifier>>,
    Opt<fieldDeclarationList>
  >;
};

//------------------------------------------------------------------------------

struct classSpecifier : public NodeMaker<classSpecifier> {
  using pattern = Seq<
    Keyword<"class">,
    Opt<LogTypename<NodeIdentifier>>,
    Opt<fieldDeclarationList>
  >;
};

//------------------------------------------------------------------------------

struct enumerator : public NodeMaker<enumerator> {
  using pattern = Seq<
    NodeIdentifier,
    Opt<Seq<Atom<'='>, NodeIdentifier>>
  >;
};

//------------------------------------------------------------------------------

struct enumeratorList : public NodeMaker<enumeratorList> {
  using pattern = Seq<
    Atom<'{'>,
    comma_separated<enumerator>,
    Opt<Atom<','>>,
    Atom<'}'>
  >;
};

//------------------------------------------------------------------------------

struct enumSpecifier : public NodeMaker<enumSpecifier> {
  using pattern = Seq<
    Keyword<"enum">,
    Opt<LogTypename<NodeIdentifier>>,
    Opt<enumeratorList>
  >;
};

//------------------------------------------------------------------------------

struct atomicTypeSpecifier : public NodeMaker<atomicTypeSpecifier> {
  using pattern = Seq<
    Keyword<"_Atomic">,
    Atom<'('>,
    typeName,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

using typeSpecifier = Oneof<
  NodeTypeName,
  atomicTypeSpecifier,
  structSpecifier,
  unionSpecifier,
  classSpecifier,
  enumSpecifier
>;

const Token* parse_type_specifier(const Token* a, const Token* b) { return typeSpecifier::match(a, b); }

//------------------------------------------------------------------------------

struct alignmentSpecifier : public NodeMaker<alignmentSpecifier> {
  using pattern = Seq<
    Keyword<"_Alignas">,
    Atom<'('>,
    Oneof<typeName, NodeConstant>,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct declarationSpecifiers : public Wrapper<declarationSpecifiers> {
  using pattern = Some<Oneof<
    NodeKeyword<"typedef">,
    NodeKeyword<"extern">,
    NodeKeyword<"static">,
    NodeKeyword<"_Thread_local">,
    NodeKeyword<"auto">,
    NodeKeyword<"register">,
    typeSpecifier,
    typeQualifier,
    functionSpecifier,
    alignmentSpecifier
  >>;
};

//------------------------------------------------------------------------------

struct designator : public NodeMaker<designator> {
  using pattern = Oneof<
    Seq<Atom<'['>, NodeConstant, Atom<']'>>,
    Seq<Atom<'.'>, NodeIdentifier>
  >;
};

//------------------------------------------------------------------------------

struct designatorList : public NodeMaker<designatorList> {
  using pattern = Some<designator>;
};

//------------------------------------------------------------------------------

struct designation : public NodeMaker<designation> {
  using pattern = Seq<designatorList, Atom<'='>>;
};

//------------------------------------------------------------------------------

struct initializer;

using initializerList =
Seq<
  Opt<designation>,
  initializer,
  Any<Seq<
    Atom<','>,
    Opt<designation>,
    initializer
  >>
>;

//------------------------------------------------------------------------------

struct initializer : public NodeMaker<initializer> {
  using pattern = Oneof<
    Ref<parse_expression>,
    Seq<
      Atom<'{'>,
      initializerList,
      Opt<Atom<','>>,
      Atom<'}'>
    >
  >;
};

const Token* parse_initializer(const Token* a, const Token* b) { return initializer::match(a, b); }

//------------------------------------------------------------------------------

using initDeclarator = Seq<
  declarator,
  Opt<Seq<
    Atom<'='>,
    initializer
  >>
>;

//------------------------------------------------------------------------------

using initDeclaratorList = Seq<
  initDeclarator,
  Opt<Seq<
    Atom<','>,
    initDeclarator
  >>
>;

//------------------------------------------------------------------------------
// this has to go around declaration.... er, how?

template<typename P>
struct store_typedef2 {
  static const Token* match(const Token* a, const Token* b) {
    if (auto end = P::match(a, b)) {
      // Poke through the node on the top of the stack to find identifiers
      auto n1 = NodeBase::node_stack.back();
      if (!n1) return end;
      if (!n1->child<NodeKeyword<"typedef">>()) {
        return end;
      }

      if (auto n2 = n1->child<declarator>()) {
        if (auto n3 = n2->child<NodeIdentifier>()) {
          auto l = n3->tok_a->lex;
          auto s = std::string(l->span_a, l->span_b);
          NodeBase::global_types.insert(s);
        }
      }
      return end;
    }
    else {
      return nullptr;
    }
  }
};

//------------------------------------------------------------------------------

struct declaration : public NodeMaker<declaration> {
  using pattern = Seq<
    declarationSpecifiers,
    Opt<initDeclaratorList>,
    Atom<';'>
  >;
};

const Token* parse_declaration(const Token* a, const Token* b) {
  return declaration::match(a, b);
}

//------------------------------------------------------------------------------

struct functionDefinition : public NodeMaker<functionDefinition> {
  using pattern = Seq<
    Opt<declarationSpecifiers>,
    declarator,
    Opt<Some<Ref<parse_declaration>>>,
    Ref<parse_statement_compound>
  >;
};

//------------------------------------------------------------------------------

using externalDeclaration =
Oneof<
  functionDefinition,
  Ref<parse_declaration>,
  Atom<';'>
>;

const Token* parse_external_declaration(const Token* a, const Token* b) {
  return externalDeclaration::match(a, b);
}

//------------------------------------------------------------------------------

const Token* parse_abstract_declarator(const Token* a, const Token* b) { return abstractDeclarator::match(a, b); }
