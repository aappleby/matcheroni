#include "c99_parser.h"
#include "Matcheroni.h"
#include "Node.h"

//------------------------------------------------------------------------------

struct abstractDeclarator;
struct alignmentQualifier;
struct atomicTypeSpecifier;
struct classSpecifier;
struct declarationSpecifier;
struct declarationQualifier;
struct declarator;
struct enumSpecifier;
struct initializer;
struct typeName;
struct structDefinition;
struct structDeclaration;
struct unionSpecifier;

//------------------------------------------------------------------------------

/*
struct NodeSpecifier : public NodeMaker<NodeSpecifier> {
  using pattern = Oneof<
    Keyword<"extern">,
    Keyword<"static">,
    Keyword<"register">,
    Keyword<"inline">,
    Keyword<"thread_local">,
    Keyword<"const">,
    Keyword<"volatile">,
    Keyword<"restrict">,
    Keyword<"__restrict__">,
    Keyword<"_Atomic">,
    Keyword<"_Noreturn">,
    Keyword<"mutable">,
    Keyword<"constexpr">,
    Keyword<"constinit">,
    Keyword<"consteval">,
    Keyword<"virtual">,
    Keyword<"explicit">
  >;
};

//------------------------------------------------------------------------------

struct NodeFieldList : public NodeMaker<NodeFieldList> {
  using pattern = Seq<
    Atom<'{'>,
    Any<Oneof<
      NodeAccessSpecifier,
      NodeConstructor,
      NodeEnum,
      NodeFunction,
      NodeStatementDeclaration
    >>,
    Atom<'}'>
  >;
};


struct NodeSpecifierList : public NodeMaker<NodeSpecifierList> {
  using pattern = Some<NodeSpecifier>;
};


//------------------------------------------------------------------------------

struct NodeDeclarationTemplate : public NodeMaker<NodeDeclarationTemplate> {
  using pattern = Seq<
    Keyword<"template">,
    NodeTemplateParams,
    NodeClass
  >;
};


struct NodeTemplateArgs : public NodeMaker<NodeTemplateArgs> {
  using pattern = Delimited<'<', '>',
    opt_comma_separated<Ref<parse_expression>>
  >;
};

struct NodeTemplateParams : public NodeMaker<NodeTemplateParams> {
  using pattern = Delimited<'<', '>', comma_separated<NodeDeclaration>>;
};
*/

//------------------------------------------------------------------------------

/*
struct NodeAccessSpecifier : public NodeMaker<NodeAccessSpecifier> {
  using pattern = Seq<
    Oneof<
      Keyword<"public">,
      Keyword<"private">
    >,
    Atom<':'>
  >;
};

struct NodeConstructor : public NodeMaker<NodeConstructor> {
  using pattern = Seq<
    NodeIdentifier,
    NodeDeclList,
    Opt<NodeInitializerList>,
    Oneof<
      NodeStatementCompound,
      Atom<';'>
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeFunction : public NodeMaker<NodeFunction> {
  using pattern = Seq<
    NodeDecltype,
    NodeIdentifier,
    NodeDeclList,
    Opt<Keyword<"const">>,
    Oneof<
      NodeStatementCompound,
      Atom<';'>
    >
  >;
};

*/

//------------------------------------------------------------------------------

struct typeQualifier : public NodeMaker<typeQualifier> {
  using pattern = Oneof<
    Keyword<"const">,
    Keyword<"restrict">,
    Keyword<"volatile">
  >;
};

struct alignmentQualifier : public NodeMaker<alignmentQualifier> {
  using pattern = Seq<
    Keyword<"_Alignas">, Atom<'('>, Oneof<typeName, NodeConstant>, Atom<')'>
  >;
};

struct functionQualifier : public NodeMaker<functionQualifier> {
  using pattern = Oneof<
    Keyword<"inline">,
    Keyword<"_Noreturn">,
    Keyword<"__inline__">,
    Keyword<"__stdcall">,
    Seq<Keyword<"__declspec">, Atom<'('>, NodeIdentifier, Atom<')'>>
  >;
};

struct declarationQualifier : public PatternWrapper<declarationQualifier> {
  using pattern = Oneof<
    typeQualifier,
    alignmentQualifier,
    functionQualifier,
    NodeKeyword<"typedef">,
    NodeKeyword<"extern">,
    NodeKeyword<"static">,
    NodeKeyword<"_Thread_local">,
    NodeKeyword<"auto">,
    NodeKeyword<"register">
  >;
};

struct declarationQualifiers : public NodeMaker<declarationQualifiers> {
  using pattern = Any<declarationQualifier>;

  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<declarationQualifiers>::match(a, b);
    return end;
  }
};

//------------------------------------------------------------------------------

struct pointer : public NodeMaker<pointer> {
  using pattern =
  Seq<
    NodeOperator<"*">,
    Any<Oneof<
      NodeOperator<"*">,
      typeQualifier
    >>
  >;
};

//------------------------------------------------------------------------------

struct parameterDeclaration : public NodeMaker<parameterDeclaration> {
  using pattern = Oneof<
    Seq<
      Some<Oneof<declarationSpecifier,declarationQualifier>>,
      Opt<Oneof<
        declarator,
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

struct arraySuffix : public PatternWrapper<arraySuffix> {
  using pattern = Oneof<
    Seq<Atom<'['>, Any<typeQualifier>, Opt<Ref<parse_expression>>, Atom<']'>>,
    Seq<Atom<'['>, Keyword<"static">, Any<typeQualifier>, Ref<parse_expression>, Atom<']'>>,
    Seq<Atom<'['>, Some<typeQualifier>, Keyword<"static">, Ref<parse_expression>, Atom<']'>>,
    Seq<Atom<'['>, Any<typeQualifier>, Atom<'*'>, Atom<']'>>
  >;
};

//------------------------------------------------------------------------------

struct declarationSpecifier : public NodeMaker<declarationSpecifier> {
  using pattern = Oneof<
    NodeTypeName,
    atomicTypeSpecifier,
    CleanDeadNodes<structDefinition>,
    CleanDeadNodes<structDeclaration>,
    unionSpecifier,
    classSpecifier,
    enumSpecifier
  >;

  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<declarationSpecifier>::match(a, b);
    return end;
  }
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
      Seq<Atom<'('>, abstractDeclarator, Atom<')'>>
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
      Seq<Opt<pointer>, Atom<'('>, declarator, Atom<')'>>
    >,
    Any<Oneof<
      arraySuffix,
      parameterList,
      identifierList,
      emptyList
    >>
  >;

  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<declarator>::match(a, b);
    return end;
  }
};

//------------------------------------------------------------------------------

struct fieldDeclaration : public NodeMaker<fieldDeclaration> {
  using pattern = Seq<
    declarationQualifiers,
    //specifierQualifierList,
    //atomicTypeSpecifier,
    declarationSpecifier,
    Opt<comma_separated<Seq< declarator, Opt<bit_suffix> >>>,
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

//----------------------------------------

/*
struct NodeNamespace : public NodeMaker<NodeNamespace> {
  using pattern = Seq<
    Keyword<"namespace">,
    NodeIdentifier,
    Opt<NodeFieldList>,
    Atom<';'>
  >;
};
*/

//------------------------------------------------------------------------------

struct structDeclaration : public NodeMaker<structDeclaration> {
  using pattern = Seq<
    Keyword<"struct">,
    LogTypename<NodeIdentifier>
  >;
};

struct structDefinition : public NodeMaker<structDefinition> {
  using pattern = Seq<
    Keyword<"struct">,
    Opt<LogTypename<NodeIdentifier>>,
    fieldDeclarationList
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
// FIXME should probably have a few diffeerent versions instead of all the opts

struct enumerator : public NodeMaker<enumerator> {
  using pattern = Seq<
    NodeIdentifier,
    Opt<Seq<Atom<'='>, NodeIdentifier>>
  >;
};

struct enumeratorList : public NodeMaker<enumeratorList> {
  using pattern = Seq<
    Atom<'{'>,
    comma_separated<enumerator>,
    Opt<Atom<','>>,
    Atom<'}'>
  >;
};

/*
struct NodeEnum : public NodeMaker<NodeEnum> {
  using pattern = Seq<
    Keyword<"enum">,
    Opt<Keyword<"class">>,
    Opt<NodeIdentifier>,
    Opt<Seq<Atom<':'>, NodeDecltype>>,
    Opt<Seq<
      Atom<'{'>,
      comma_separated<Ref<parse_expression>>,
      Atom<'}'>
    >>,
    Opt<NodeIdentifier>,
    Atom<';'>
  >;
};
*/

struct enumSpecifier : public NodeMaker<enumSpecifier> {
  using pattern = Seq<
    Keyword<"enum">,
    Opt<Keyword<"class">>,
    Opt<LogTypename<NodeIdentifier>>,
    Opt<Seq<Atom<':'>, typeName>>,
    Opt<enumeratorList>
  >;
};

//------------------------------------------------------------------------------

struct typeName : public NodeMaker<typeName> {
  using pattern = Seq<
    //specifierQualifierList,
    declarationQualifiers,
    declarationSpecifier,
    Opt<abstractDeclarator>
  >;
};


struct atomicTypeSpecifier : public NodeMaker<atomicTypeSpecifier> {
  using pattern = Seq<
    Keyword<"_Atomic">,
    Atom<'('>,
    typeName,
    Atom<')'>
  >;
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

struct initializerList : public PatternWrapper<initializerList> {

  static const Token* match(const Token* a, const Token* b) {
    auto end = PatternWrapper<initializerList>::match(a, b);
    return end;
  }

  using pattern =
  Seq<
    Opt<designation>,
    initializer,
    Any<Seq<
      Atom<','>,
      Opt<designation>,
      initializer
    >>
  >;
};

//------------------------------------------------------------------------------

struct initializer : public NodeMaker<initializer> {

  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<initializer>::match(a, b);
    return end;
  }

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

//------------------------------------------------------------------------------

struct functionDefinition : public NodeMaker<functionDefinition> {
  using pattern = Seq<
    Any<declarationQualifier>,
    declarationSpecifier,
    declarator,
    Opt<Some<Ref<parse_declaration>>>,
    Ref<parse_statement_compound>
  >;
};

//------------------------------------------------------------------------------

struct externalDeclaration : public PatternWrapper<externalDeclaration> {
  using pattern =
  Oneof<
    functionDefinition,
    Ref<parse_declaration>,
    Atom<';'>
  >;
};

const Token* parse_external_declaration(const Token* a, const Token* b) {
  return externalDeclaration::match(a, b);
}

//------------------------------------------------------------------------------
// this has to go around declaration below

template<typename P>
struct store_typedef2 {
  static const Token* match(const Token* a, const Token* b) {
    if (auto end = P::match(a, b)) {
      // Poke through the node on the top of the stack to find identifiers
      auto n1 = NodeBase::node_stack.back();
      if (!n1) return end;
      if (!n1->search<NodeKeyword<"typedef">>()) {
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
    declarationQualifiers,
    declarationSpecifier,
    Opt<comma_separated<
      Seq<declarator, Opt<Seq<Atom<'='>, initializer>>>
    >>,
    Atom<';'>
  >;
};

const Token* parse_declaration(const Token* a, const Token* b) {
  return store_typedef2<declaration>::match(a, b);
}

//------------------------------------------------------------------------------
