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
const Token* parse_direct_abstract_declarator(const Token* a, const Token* b);
const Token* parse_initializer               (const Token* a, const Token* b);
const Token* parse_statement_compound        (const Token* a, const Token* b);

//------------------------------------------------------------------------------

struct functionSpecifier : public NodeMaker<functionSpecifier> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;

  using pattern = Oneof<
    Keyword<"inline">,
    Keyword<"_Noreturn">,
    Keyword<"__inline__">,
    Keyword<"__stdcall">,
    Seq<Keyword<"__declspec">, Atom<'('>, Ref<parse_identifier>, Atom<')'>>
  >;
};

//------------------------------------------------------------------------------

struct storageClassSpecifier : public NodeMaker<storageClassSpecifier> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;

  using pattern = Oneof<
    Keyword<"typedef">,
    Keyword<"extern">,
    Keyword<"static">,
    Keyword<"_Thread_local">,
    Keyword<"auto">,
    Keyword<"register">
  >;
};

//------------------------------------------------------------------------------

struct typeQualifier : public NodeMaker<typeQualifier> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;

  using pattern = Oneof<
    Keyword<"const">,
    Keyword<"restrict">,
    Keyword<"volatile">,
    Keyword<"_Atomic">
  >;
};

//------------------------------------------------------------------------------

struct builtinTypes : public NodeMaker<builtinTypes> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;

  using pattern = Oneof<
    Keyword<"void">,
    Keyword<"char">,
    Keyword<"short">,
    Keyword<"int">,
    Keyword<"long">,
    Keyword<"float">,
    Keyword<"double">,
    Keyword<"signed">,
    Keyword<"unsigned">,
    Keyword<"_Bool">,
    Keyword<"_Complex">,
    Keyword<"__m128">,
    Keyword<"__m128d">,
    Keyword<"__m128i">
  >;
};

//------------------------------------------------------------------------------

struct typeQualifierList : public NodeMaker<typeQualifierList> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;

  using pattern = Some<typeQualifier>;
};

//------------------------------------------------------------------------------

struct pointer : public NodeMaker<pointer> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;
  using pattern = Some<Seq<
    Atom<'*'>,
    Opt<typeQualifierList>
  >>;
};

//------------------------------------------------------------------------------

struct parameterDeclaration : public NodeMaker<parameterDeclaration> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;
  using pattern = Oneof<
    Seq<Ref<parse_declaration_specifiers>, Ref<parse_declarator>>,
    Seq<Ref<parse_declaration_specifiers>, Opt<Ref<parse_abstract_declarator>>>
  >;
};

//------------------------------------------------------------------------------

struct parameterList : public NodeMaker<parameterList> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;
  using pattern = Seq<
    parameterDeclaration,
    Any<Seq<
      Atom<','>,
      parameterDeclaration
    >>
  >;
};

//------------------------------------------------------------------------------

struct parameterTypeList : public NodeMaker<parameterTypeList> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;
  using pattern = Seq<
    parameterList,
    Opt<Seq<
      Atom<','>,
      Operator<"...">
    >>
  >;
};

//------------------------------------------------------------------------------

struct abstractDeclarator : public NodeMaker<abstractDeclarator> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;
  using pattern = Oneof<
    pointer,
    Seq<
      Opt<pointer>,
      Ref<parse_direct_abstract_declarator>
    >
  >;
};
const Token* parse_abstract_declarator(const Token* a, const Token* b) { return abstractDeclarator::match(a, b); }

//------------------------------------------------------------------------------

struct directAbstractDeclarator : public NodeMaker<directAbstractDeclarator> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;
  using pattern = Oneof<
    Seq<Atom<'('>, abstractDeclarator, Atom<')'>>,
    Some<Oneof<
      Seq<Atom<'['>, Opt<typeQualifierList>, Opt<Ref<parse_expression>>, Atom<']'>>,
      Seq<Atom<'['>, Keyword<"static">, Opt<typeQualifierList>, Ref<parse_expression>,  Atom<']'>>,
      Seq<Atom<'['>, typeQualifierList, Keyword<"static">, Ref<parse_expression>, Atom<']'>>,
      Seq<Atom<'['>, Atom<'*'>, Atom<']'>>,
      Seq<Atom<'('>, Opt<parameterTypeList>, Atom<')'>>
    >>
  >;
};
const Token* parse_direct_abstract_declarator(const Token* a, const Token* b) { return directAbstractDeclarator::match(a, b); }

//------------------------------------------------------------------------------

struct specifierQualifierList : public NodeMaker<specifierQualifierList> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;

  using pattern = Some<Oneof<
    Ref<parse_type_specifier>,
    typeQualifier
  >>;
};

//------------------------------------------------------------------------------

struct typeName : public NodeMaker<typeName> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;
  using pattern = Seq<
    specifierQualifierList,
    Opt<abstractDeclarator>
  >;
};

//------------------------------------------------------------------------------

struct identifierList : public NodeMaker<identifierList> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;
  using pattern = Seq<
    Ref<parse_identifier>,
    Opt<Seq<
      Atom<','>,
      Ref<parse_identifier>
    >>
  >;
};

//------------------------------------------------------------------------------

struct directDeclaratorSuffix : public NodeMaker<directDeclaratorSuffix> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;
  using pattern = Oneof<
    Seq<Atom<'['>, Opt<typeQualifierList>, Opt<Ref<parse_expression>>, Atom<']'>>,
    Seq<Atom<'['>, Keyword<"static">, Opt<typeQualifierList>, Ref<parse_expression>, Atom<']'>>,
    Seq<Atom<'['>, typeQualifierList, Keyword<"static">, Ref<parse_expression>, Atom<']'>>,
    Seq<Atom<'['>, Opt<typeQualifierList>, Atom<'*'>, Atom<']'>>,
    Seq<Atom<'('>, parameterTypeList, Atom<')'>>,
    Seq<Atom<'('>, Opt<identifierList>, Atom<')'>>
  >;
};

//------------------------------------------------------------------------------

/*
struct directDeclarator : public NodeMaker<directDeclarator> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;

  using pattern = Seq<
    Oneof<
      Ref<parse_identifier>,
      Seq<Ref<parse_identifier>, Atom<':'>, Ref<parse_constant>>, // bit field
      Seq<Atom<'('>, Ref<parse_declarator>, Atom<')'>>
    >,
    Any<directDeclaratorSuffix>
  >;
};
*/

using directDeclarator =
Seq<
  Oneof<
    Ref<parse_identifier>,
    Seq<Ref<parse_identifier>, Atom<':'>, Ref<parse_constant>>, // bit field
    Seq<Atom<'('>, Ref<parse_declarator>, Atom<')'>>
  >,
  Any<directDeclaratorSuffix>
>;


//------------------------------------------------------------------------------

struct declarator : public NodeMaker<declarator> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;

  using pattern = Seq<
    Opt<pointer>,
    directDeclarator
  >;
};
const Token* parse_declarator(const Token* a, const Token* b) { return declarator::match(a, b); }

//------------------------------------------------------------------------------

struct structOrUnionOrClass : public NodeMaker<structOrUnionOrClass> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;

  using pattern = Oneof<
    Keyword<"struct">,
    Keyword<"union">,
    Keyword<"class">
  >;
};

//------------------------------------------------------------------------------

struct structDeclarator : public NodeMaker<structDeclarator> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;

  using pattern = Oneof<
    declarator,
    Seq<Opt<declarator>, Atom<':'>, Ref<parse_constant>>
  >;
};

//------------------------------------------------------------------------------

/*
struct structDeclaratorList : public NodeMaker<structDeclaratorList> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;

  using pattern = Seq<
    structDeclarator,
    Any<Seq<
      Atom<','>,
      structDeclarator
    >>
  >;
};
*/

using structDeclaratorList = Seq<
  structDeclarator,
  Any<Seq<
    Atom<','>,
    structDeclarator
  >>
>;

//------------------------------------------------------------------------------

struct structDeclaration : public NodeMaker<structDeclaration> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;

  using pattern = Oneof<
    Seq<specifierQualifierList, structDeclaratorList, Atom<';'>>,
    Seq<specifierQualifierList, Atom<';'>>
  >;
};

//------------------------------------------------------------------------------

struct structDeclarationList : public NodeMaker<structDeclarationList> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;

  using pattern = Some<structDeclaration>;
};

//------------------------------------------------------------------------------

struct structOrUnionSpecifier : public NodeMaker<structOrUnionSpecifier> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;

  using pattern = Oneof<
    Seq<structOrUnionOrClass, Opt<Ref<parse_identifier>>, Atom<'{'>, structDeclarationList, Atom<'}'>>,
    Seq<structOrUnionOrClass, Ref<parse_identifier>>
  >;
};

//------------------------------------------------------------------------------

/*
struct enumerationConstant : public NodeMaker<enumerationConstant> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;

  using pattern = Ref<parse_identifier>;
};
*/

using enumerationConstant = Ref<parse_identifier>;

//------------------------------------------------------------------------------

struct enumerator : public NodeMaker<enumerator> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;

  using pattern = Seq<
    enumerationConstant,
    Opt<Seq<
      Atom<'='>,
      Ref<parse_constant>
    >>
  >;
};

//------------------------------------------------------------------------------

struct enumeratorList : public NodeMaker<enumeratorList> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;

  using pattern = Seq<
    enumerator,
    Any<Seq<
      Atom<','>,
      enumerator
    >>
  >;
};

//------------------------------------------------------------------------------

struct enumSpecifier : public NodeMaker<enumSpecifier> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;

  using pattern = Oneof<
    Seq<Keyword<"enum">, Opt<Ref<parse_identifier>>, Atom<'{'>, enumeratorList, Opt<Atom<','>>, Atom<'}'>>,
    Seq<Keyword<"enum">, Ref<parse_identifier>>
  >;
};

//------------------------------------------------------------------------------

struct atomicTypeSpecifier : public NodeMaker<atomicTypeSpecifier> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;

  using pattern = Seq<
    Keyword<"_Atomic">,
    Atom<'('>,
    typeName,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

/*
struct typeSpecifier : public NodeMaker<typeSpecifier> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;

  using pattern = Oneof<
    builtinTypes,
    atomicTypeSpecifier,
    structOrUnionSpecifier,
    enumSpecifier,
    Ref<parse_type_name>
  >;
};
*/

using typeSpecifier = Oneof<
  builtinTypes,
  atomicTypeSpecifier,
  structOrUnionSpecifier,
  enumSpecifier,
  Ref<parse_type_name>
>;

const Token* parse_type_specifier(const Token* a, const Token* b) { return typeSpecifier::match(a, b); }

//------------------------------------------------------------------------------

struct alignmentSpecifier : public NodeMaker<alignmentSpecifier> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;

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
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;

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
  storageClassSpecifier,
  typeSpecifier,
  typeQualifier,
  functionSpecifier,
  alignmentSpecifier
>;

//------------------------------------------------------------------------------

/*
struct declarationSpecifiers : public NodeMaker<declarationSpecifiers> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;
  using pattern = Some<declarationSpecifier>;
};
*/

using declarationSpecifiers = Some<declarationSpecifier>;

const Token* parse_declaration_specifiers(const Token* a, const Token* b) { return declarationSpecifiers::match(a, b); }

//------------------------------------------------------------------------------

struct designator : public NodeMaker<designator> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;
  using pattern = Oneof<
    Seq<Atom<'['>, Ref<parse_constant>, Atom<']'>>,
    Seq<Atom<'.'>, Ref<parse_identifier>>
  >;
};

//------------------------------------------------------------------------------

struct designatorList : public NodeMaker<designatorList> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;
  using pattern = Some<designator>;
};

//------------------------------------------------------------------------------

struct designation : public NodeMaker<designation> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;
  using pattern = Seq<designatorList, Atom<'='>>;
};

//------------------------------------------------------------------------------

/*
struct initializerList : public NodeMaker<initializerList> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;

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
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;

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
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;

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
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;

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
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;

  using pattern = Seq<
    declarationSpecifiers,
    Opt<initDeclaratorList>,
    Atom<';'>
  >;
};

const Token* parse_declaration(const Token* a, const Token* b) {
  return declaration::match(a, b);
}

struct functionDefinition : public NodeMaker<functionDefinition> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_INVALID;

  using pattern = Seq<
    Opt<declarationSpecifiers>,
    declarator,
    Opt<Some<declaration>>,
    Ref<parse_statement_compound>
  >;
};

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
