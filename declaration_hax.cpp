#include "Matcheroni.h"
#include "Node.h"

const Token* parse_expression (const Token* a, const Token* b);
const Token* parse_identifier (const Token* a, const Token* b);
const Token* parse_constant   (const Token* a, const Token* b);
const Token* parse_type_name  (const Token* a, const Token* b);

//------------------------------------------------------------------------------

const Token* parse_abstract_declarator       (const Token* a, const Token* b);
const Token* parse_declarator                (const Token* a, const Token* b);
const Token* parse_declaration_specifiers    (const Token* a, const Token* b);
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
    Seq<Keyword<"__declspec">, Atom<'('>, Ref<parse_identifier>, Atom<')'>>
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
      Ref<parse_declaration_specifiers>,
      Opt<Oneof<
        Ref<parse_declarator>,
        Ref<parse_abstract_declarator>
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
using identifierList = Seq<Atom<'('>, comma_separated<Ref<parse_identifier>>, Atom<')'>>;
using emptyList      = Seq<Atom<'('>, Atom<')'>>;

//------------------------------------------------------------------------------

using bit_suffix = Seq<Atom<':'>, Ref<parse_constant>>;

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
      Seq<Opt<pointer>, Ref<parse_identifier>, Opt<bit_suffix>>,
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
    Opt<Ref<parse_identifier>>,
    Opt<fieldDeclarationList>
  >;
};

//------------------------------------------------------------------------------

struct unionSpecifier : public NodeMaker<unionSpecifier> {
  using pattern = Seq<
    Keyword<"union">,
    Opt<Ref<parse_identifier>>,
    Opt<fieldDeclarationList>
  >;
};

//------------------------------------------------------------------------------

struct classSpecifier : public NodeMaker<classSpecifier> {
  using pattern = Seq<
    Keyword<"class">,
    Opt<Ref<parse_identifier>>,
    Opt<fieldDeclarationList>
  >;
};

//------------------------------------------------------------------------------

struct enumerator : public NodeMaker<enumerator> {
  using pattern = Seq<
    Ref<parse_identifier>,
    Opt<Seq<Atom<'='>, Ref<parse_constant>>>
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
    Opt<Ref<parse_identifier>>,
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
  Ref<parse_type_name>,
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
    Oneof<typeName, Ref<parse_constant>>,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

/*
struct declarationSpecifier : public NodeMaker<declarationSpecifier> {
  using pattern = Oneof<
    storageClassSpecifier,
    typeSpecifier,
    typeQualifier,
    functionSpecifier,
    alignmentSpecifier
  >;
};
*/

using declarationSpecifier = Oneof<
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
>;

//------------------------------------------------------------------------------

/*
struct declarationSpecifiers : public NodeMaker<declarationSpecifiers> {
  using pattern = Some<declarationSpecifier>;
};
*/

using declarationSpecifiers = Some<declarationSpecifier>;

const Token* parse_declaration_specifiers(const Token* a, const Token* b) { return declarationSpecifiers::match(a, b); }

//------------------------------------------------------------------------------

struct designator : public NodeMaker<designator> {
  using pattern = Oneof<
    Seq<Atom<'['>, Ref<parse_constant>, Atom<']'>>,
    Seq<Atom<'.'>, Ref<parse_identifier>>
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

/*
struct initializerList : public NodeMaker<initializerList> {
  using pattern = Seq<
    Opt<designation>,
    Ref<parse_initializer>,
    Any<Seq<
      Atom<','>,
      Opt<designation>,
      Ref<parse_initializer>
    >>
  >;
};
*/

using initializerList =
Seq<
  Opt<designation>,
  Ref<parse_initializer>,
  Any<Seq<
    Atom<','>,
    Opt<designation>,
    Ref<parse_initializer>
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

/*
struct initDeclarator : public NodeMaker<initDeclarator> {
  using pattern = Seq<
    declarator,
    Opt<Seq<
      Atom<'='>,
      initializer
    >>
  >;
};
*/

using initDeclarator = Seq<
  declarator,
  Opt<Seq<
    Atom<'='>,
    initializer
  >>
>;

//------------------------------------------------------------------------------

/*
struct initDeclaratorList : public NodeMaker<initDeclaratorList> {
  using pattern = Seq<
    initDeclarator,
    Opt<Seq<
      Atom<','>,
      initDeclarator
    >>
  >;
};
*/

using initDeclaratorList = Seq<
  initDeclarator,
  Opt<Seq<
    Atom<','>,
    initDeclarator
  >>
>;

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
    Opt<Some<declaration>>,
    Ref<parse_statement_compound>
  >;
};

//------------------------------------------------------------------------------

using externalDeclaration =
Oneof<
  functionDefinition,
  declaration,
  Atom<';'>
>;

const Token* parse_external_declaration(const Token* a, const Token* b) {
  return externalDeclaration::match(a, b);
}

//------------------------------------------------------------------------------

const Token* parse_abstract_declarator(const Token* a, const Token* b) { return abstractDeclarator::match(a, b); }
