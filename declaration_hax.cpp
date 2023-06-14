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

using functionSpecifier = Oneof<
  Keyword<"inline">,
  Keyword<"_Noreturn">,
  Keyword<"__inline__">,
  Keyword<"__stdcall">,
  Seq<Keyword<"__declspec">, Atom<'('>, Ref<parse_identifier>, Atom<')'>>
>;

using storageClassSpecifier = Oneof<
  Keyword<"typedef">,
  Keyword<"extern">,
  Keyword<"static">,
  Keyword<"_Thread_local">,
  Keyword<"auto">,
  Keyword<"register">
>;

using typeQualifier = Oneof<
  Keyword<"const">,
  Keyword<"restrict">,
  Keyword<"volatile">,
  Keyword<"_Atomic">
>;

using builtinTypes = Oneof<
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


using typeQualifierList = Some<typeQualifier>;

using pointer = Some<Seq<
  Atom<'*'>,
  Opt<typeQualifierList>
>>;

using parameterDeclaration = Oneof<
  Seq<Ref<parse_declaration_specifiers>, Ref<parse_declarator>>,
  Seq<Ref<parse_declaration_specifiers>, Opt<Ref<parse_abstract_declarator>>>
>;

using parameterList = Seq<
  parameterDeclaration,
  Any<Seq<
    Atom<','>,
    parameterDeclaration
  >>
>;

using parameterTypeList =
Seq<
  parameterList,
  Opt<Seq<
    Atom<','>,
    Operator<"...">
  >>
>;

using abstractDeclarator =
Oneof<
  pointer,
  Seq<
    Opt<pointer>,
    Ref<parse_direct_abstract_declarator>
  >
>;
const Token* parse_abstract_declarator(const Token* a, const Token* b) { return abstractDeclarator::match(a, b); }

using directAbstractDeclarator =
Oneof<
  Seq<Atom<'('>, abstractDeclarator, Atom<')'>>,
  Some<Oneof<
    Seq<Atom<'['>, Opt<typeQualifierList>, Opt<Ref<parse_expression>>, Atom<']'>>,
    Seq<Atom<'['>, Keyword<"static">, Opt<typeQualifierList>, Ref<parse_expression>,  Atom<']'>>,
    Seq<Atom<'['>, typeQualifierList, Keyword<"static">, Ref<parse_expression>, Atom<']'>>,
    Seq<Atom<'['>, Atom<'*'>, Atom<']'>>,
    Seq<Atom<'('>, Opt<parameterTypeList>, Atom<')'>>
  >>
>;
const Token* parse_direct_abstract_declarator(const Token* a, const Token* b) { return directAbstractDeclarator::match(a, b); }

using specifierQualifierList =
Some<Oneof<
  Ref<parse_type_specifier>,
  typeQualifier
>>;

using typeName = Seq<
  specifierQualifierList,
  Opt<abstractDeclarator>
>;

using identifierList =
Seq<
  Ref<parse_identifier>,
  Opt<Seq<
    Atom<','>,
    Ref<parse_identifier>
  >>
>;

using directDeclaratorSuffix = Oneof<
  Seq<Atom<'['>, Opt<typeQualifierList>, Opt<Ref<parse_expression>>, Atom<']'>>,
  Seq<Atom<'['>, Keyword<"static">, Opt<typeQualifierList>, Ref<parse_expression>, Atom<']'>>,
  Seq<Atom<'['>, typeQualifierList, Keyword<"static">, Ref<parse_expression>, Atom<']'>>,
  Seq<Atom<'['>, Opt<typeQualifierList>, Atom<'*'>, Atom<']'>>,
  Seq<Atom<'('>, parameterTypeList, Atom<')'>>,
  Seq<Atom<'('>, Opt<identifierList>, Atom<')'>>
>;

using directDeclarator = Seq<
  Oneof<
    Ref<parse_identifier>,
    Seq<Ref<parse_identifier>, Atom<':'>, Ref<parse_constant>>, // bit field
    Seq<Atom<'('>, Ref<parse_declarator>, Atom<')'>>
  >,
  Any<directDeclaratorSuffix>
>;

using declarator =
Seq<
  Opt<pointer>,
  directDeclarator
>;
const Token* parse_declarator(const Token* a, const Token* b) { return declarator::match(a, b); }

using structOrUnionOrClass = Oneof<
  Keyword<"struct">,
  Keyword<"union">,
  Keyword<"class">
>;

using structDeclarator =
Oneof<
  declarator,
  Seq<Opt<declarator>, Atom<':'>, Ref<parse_constant>>
>;

using structDeclaratorList = Seq<
  structDeclarator,
  Any<Seq<
    Atom<','>,
    structDeclarator
  >>
>;

using structDeclaration =
Oneof<
  Seq<specifierQualifierList, structDeclaratorList, Atom<';'>>,
  Seq<specifierQualifierList, Atom<';'>>
>;

using structDeclarationList = Some<structDeclaration>;

using structOrUnionSpecifier =
Oneof<
  Seq<structOrUnionOrClass, Opt<Ref<parse_identifier>>, Atom<'{'>, structDeclarationList, Atom<'}'>>,
  Seq<structOrUnionOrClass, Ref<parse_identifier>>
>;

using enumerationConstant = Ref<parse_identifier>;

using enumerator =
Seq<
  enumerationConstant,
  Opt<Seq<
    Atom<'='>,
    Ref<parse_constant>
  >>
>;

using enumeratorList =
Seq<
  enumerator,
  Any<Seq<
    Atom<','>,
    enumerator
  >>
>;

using enumSpecifier =
Oneof<
  Seq<Keyword<"enum">, Opt<Ref<parse_identifier>>, Atom<'{'>, enumeratorList, Opt<Atom<','>>, Atom<'}'>>,
  Seq<Keyword<"enum">, Ref<parse_identifier>>
>;

using atomicTypeSpecifier = Seq<
  Keyword<"_Atomic">,
  Atom<'('>,
  typeName,
  Atom<')'>
>;

using typeSpecifier =
Oneof<
  builtinTypes,
  atomicTypeSpecifier,
  structOrUnionSpecifier,
  enumSpecifier,
  Ref<parse_type_name>
>;
const Token* parse_type_specifier(const Token* a, const Token* b) { return typeSpecifier::match(a, b); }

using alignmentSpecifier =
Seq<
  Keyword<"_Alignas">,
  Atom<'('>,
  Oneof<typeName, Ref<parse_constant>>,
  Atom<')'>
>;

using declarationSpecifier =
Oneof<
  storageClassSpecifier,
  typeSpecifier,
  typeQualifier,
  functionSpecifier,
  alignmentSpecifier
>;

using declarationSpecifiers = Some<declarationSpecifier>;
const Token* parse_declaration_specifiers(const Token* a, const Token* b) { return declarationSpecifiers::match(a, b); }

using designator =
Oneof<
  Seq<Atom<'['>, Ref<parse_constant>, Atom<']'>>,
  Seq<Atom<'.'>, Ref<parse_identifier>>
>;

using designatorList = Some<designator>;

using designation = Seq<designatorList, Atom<'='>>;

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

using initializer =
Oneof<
  Ref<parse_expression>,
  Seq<
    Atom<'{'>,
    initializerList,
    Opt<Atom<','>>,
    Atom<'}'>
  >
>;
const Token* parse_initializer(const Token* a, const Token* b) { return initializer::match(a, b); }

using initDeclarator =
Seq<
  declarator,
  Opt<Seq<
    Atom<'='>,
    initializer
  >>
>;

using initDeclaratorList =
Seq<
  initDeclarator,
  Opt<Seq<
    Atom<','>,
    initDeclarator
  >>
>;

using declaration =
Seq<
  declarationSpecifiers,
  Opt<initDeclaratorList>,
  Atom<';'>
>;
const Token* parse_declaration(const Token* a, const Token* b) {
  return declaration::match(a, b);
}
