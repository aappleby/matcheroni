#include "c99_parser.h"
#include "Matcheroni.h"
#include "Node.h"

//------------------------------------------------------------------------------

struct NodeAbstractDeclarator;
struct NodeAtomicType;
struct NodeSpecifier;
struct NodeDeclarator;
struct NodeEnum;
struct NodeInitializer;
struct NodeTypeDecl;
struct NodeConstructor;
struct NodeFunction;
struct NodeVariable;

//------------------------------------------------------------------------------

struct NodeTypeQualifier : public NodeMaker<NodeTypeQualifier> {
  using pattern = Oneof<
    NodeKeyword<"const">,
    NodeKeyword<"mutable">,
    NodeKeyword<"restrict">,
    NodeKeyword<"volatile">
  >;
};

struct NodeAlignmentQualifier : public NodeMaker<NodeAlignmentQualifier> {
  using pattern = Seq<
    NodeKeyword<"_Alignas">, Atom<'('>, Oneof<NodeTypeDecl, NodeConstant>, Atom<')'>
  >;
};

struct NodeFunctionQualifier : public NodeMaker<NodeFunctionQualifier> {
  using pattern = Oneof<
    NodeKeyword<"explicit">,
    NodeKeyword<"inline">,
    NodeKeyword<"_Noreturn">,
    NodeKeyword<"__inline__">,
    NodeKeyword<"__stdcall">,
    Seq<NodeKeyword<"__declspec">, Atom<'('>, NodeIdentifier, Atom<')'>>
  >;
};

struct NodeQualifier : public PatternWrapper<NodeQualifier> {
  using pattern = Oneof<
    NodeTypeQualifier,
    NodeAlignmentQualifier,
    NodeFunctionQualifier,
    NodeKeyword<"thread_local">,
    NodeKeyword<"virtual">,
    NodeKeyword<"constexpr">,
    NodeKeyword<"constinit">,
    NodeKeyword<"consteval">,
    NodeKeyword<"typedef">,
    NodeKeyword<"extern">,
    NodeKeyword<"static">,
    NodeKeyword<"_Thread_local">,
    NodeKeyword<"auto">,
    NodeKeyword<"register">,
    NodeKeyword<"__restrict__">
  >;
};

struct NodeQualifiers : public NodeMaker<NodeQualifiers> {
  using pattern = Any<NodeQualifier>;

  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<NodeQualifiers>::match(a, b);
    return end;
  }
};

//------------------------------------------------------------------------------

struct NodePointer : public NodeMaker<NodePointer> {
  using pattern =
  Seq<
    NodeOperator<"*">,
    Any<Oneof<
      NodeOperator<"*">,
      NodeTypeQualifier
    >>
  >;
};

//------------------------------------------------------------------------------

struct NodeParam : public NodeMaker<NodeParam> {
  using pattern = Oneof<
    Seq<
      NodeQualifiers,
      NodeSpecifier,
      Opt<Oneof<
        NodeDeclarator,
        NodeAbstractDeclarator
      >>
    >,
    NodeOperator<"...">
  >;
};

struct NodeParamList : public NodeMaker<NodeParamList> {
  using pattern = Seq<
    Atom<'('>,
    Opt<comma_separated<NodeParam>>,
    Atom<')'>
  >;

  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<NodeParamList>::match(a, b);
    return end;
  }
};

//------------------------------------------------------------------------------

struct NodeArraySuffix : public NodeMaker<NodeArraySuffix> {
  using pattern = Oneof<
    Seq<Atom<'['>, Any<NodeTypeQualifier>, Opt<Ref<parse_expression>>, Atom<']'>>,
    Seq<Atom<'['>, Keyword<"static">, Any<NodeTypeQualifier>, Ref<parse_expression>, Atom<']'>>,
    Seq<Atom<'['>, Some<NodeTypeQualifier>, Keyword<"static">, Ref<parse_expression>, Atom<']'>>,
    Seq<Atom<'['>, Any<NodeTypeQualifier>, Atom<'*'>, Atom<']'>>
  >;
};

//------------------------------------------------------------------------------

struct NodeTemplateArgs : public NodeMaker<NodeTemplateArgs> {
  using pattern = Delimited<'<', '>',
    opt_comma_separated<Ref<parse_expression>>
  >;
};

//------------------------------------------------------------------------------

struct NodeSpecifier : public NodeMaker<NodeSpecifier> {
  using pattern = Seq<
    Opt<Keyword<"struct">>,
    Oneof<
      NodeGlobalType,
      NodeAtomicType
    >,
    Opt<NodeTemplateArgs>
  >;


  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<NodeSpecifier>::match(a, b);
    return end;
  }
};

//------------------------------------------------------------------------------

struct NodeBitSuffix : public NodeMaker<NodeBitSuffix> {
  using pattern = Seq<Atom<':'>, NodeConstant>;
};

//------------------------------------------------------------------------------

struct NodeAbstractDeclarator : public NodeMaker<NodeAbstractDeclarator> {
  using pattern = Oneof<
    NodePointer,
    Seq<
      Opt<NodePointer>,
      Seq<Atom<'('>, NodeAbstractDeclarator, Atom<')'>>
    >,
    Seq<
      Opt<NodePointer>,
      Some<Oneof<
        NodeArraySuffix,
        NodeParamList
      >>
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeDeclarator : public NodeMaker<NodeDeclarator> {
  using pattern = Seq<
    Oneof<
      Seq<Opt<NodePointer>, NodeIdentifier, Opt<NodeBitSuffix>>,
      Seq<Opt<NodePointer>, Atom<'('>, NodeDeclarator, Atom<')'>>
    >,
    Any<Oneof<
      NodeArraySuffix,
      NodeParamList
    >>
  >;

  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<NodeDeclarator>::match(a, b);
    return end;
  }
};

//------------------------------------------------------------------------------

struct NodeAccessSpecifier : public NodeMaker<NodeAccessSpecifier> {
  using pattern = Seq<
    Oneof<
      Keyword<"public">,
      Keyword<"private">
    >,
    Atom<':'>
  >;
};

struct NodeField : public PatternWrapper<NodeField> {
  using pattern = Oneof<
    NodeAccessSpecifier,
    NodeConstructor,
    NodeEnum,
    NodeFunction,
    NodeVariable
  >;

  static const Token* match(const Token* a, const Token* b) {
    auto end = PatternWrapper<NodeField>::match(a, b);
    return end;
  }
};

struct NodeFieldList : public NodeMaker<NodeFieldList> {
  using pattern = Seq<
    Atom<'{'>,
    Any<NodeField>,
    Atom<'}'>
  >;

  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<NodeFieldList>::match(a, b);
    return end;
  }
};

//------------------------------------------------------------------------------

struct NodeNamespace : public NodeMaker<NodeNamespace> {
  using pattern = Seq<
    Keyword<"namespace">,
    Opt<NodeIdentifier>,
    Opt<NodeFieldList>
  >;
};

struct NodeStruct : public NodeMaker<NodeStruct> {
  using pattern = Seq<
    Keyword<"struct">,
    Opt<LogTypename<NodeIdentifier>>,
    Opt<NodeFieldList>
  >;
};

struct NodeUnion : public NodeMaker<NodeUnion> {
  using pattern = Seq<
    Keyword<"union">,
    Opt<LogTypename<NodeIdentifier>>,
    Opt<NodeFieldList>
  >;
};

struct NodeClass : public NodeMaker<NodeClass> {
  using pattern = Seq<
    Keyword<"class">,
    Opt<LogTypename<NodeIdentifier>>,
    Opt<NodeFieldList>
  >;
};

//------------------------------------------------------------------------------

struct NodeTemplateParams : public NodeMaker<NodeTemplateParams> {
  using pattern = Delimited<'<', '>',
    comma_separated<NodeVariable>
  >;
};

struct NodeTemplate : public NodeMaker<NodeTemplate> {
  using pattern = Seq<
    Keyword<"template">,
    NodeTemplateParams,
    NodeClass
  >;
};

//------------------------------------------------------------------------------
// FIXME should probably have a few diffeerent versions instead of all the opts

struct NodeEnumerator : public NodeMaker<NodeEnumerator> {
  using pattern = Seq<
    NodeIdentifier,
    Opt<Seq<Atom<'='>, NodeIdentifier>>
  >;
};

struct NodeEnumerators : public NodeMaker<NodeEnumerators> {
  using pattern = Seq<
    Atom<'{'>,
    comma_separated<NodeEnumerator>,
    Opt<Atom<','>>,
    Atom<'}'>
  >;
};

struct NodeEnum : public NodeMaker<NodeEnum> {
  using pattern = Seq<
    Keyword<"enum">,
    Opt<Keyword<"class">>,
    Opt<LogTypename<NodeIdentifier>>,
    Opt<Seq<Atom<':'>, NodeTypeDecl>>,
    Opt<NodeEnumerators>
    //Opt<NodeIdentifier>, fixme enum {} blah;?
  >;
};

//------------------------------------------------------------------------------

struct NodeTypeDecl : public NodeMaker<NodeTypeDecl> {
  using pattern = Seq<
    NodeQualifiers,
    NodeSpecifier,
    Opt<NodeAbstractDeclarator>
  >;
};


struct NodeAtomicType : public NodeMaker<NodeAtomicType> {
  using pattern = Seq<
    Keyword<"_Atomic">,
    Atom<'('>,
    NodeTypeDecl,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeDesignation : public NodeMaker<NodeDesignation> {
  using pattern =
  Some<Oneof<
    Seq<Atom<'['>, NodeConstant, Atom<']'>>,
    Seq<Atom<'.'>, NodeIdentifier>
  >>;
};

struct NodeInitializerList : public PatternWrapper<NodeInitializerList> {
  using pattern = Seq<
    Atom<'{'>,
    opt_comma_separated<
      Seq<
        Opt<Seq<
          NodeDesignation,
          Atom<'='>
        >>,
        NodeInitializer
      >
    >,
    Atom<'}'>
  >;
};

struct NodeInitializer : public NodeMaker<NodeInitializer> {
  using pattern = Oneof<
    NodeInitializerList,
    Ref<parse_expression>
  >;

  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<NodeInitializer>::match(a, b);
    return end;
  }
};

//------------------------------------------------------------------------------

struct NodeFunction : public NodeMaker<NodeFunction> {
  using pattern = Seq<
    NodeQualifiers,
    NodeSpecifier,
    NodeIdentifier,
    NodeParamList,
    Opt<NodeKeyword<"const">>,
    Oneof<
      Atom<';'>,
      Ref<parse_statement_compound>
    >
  >;

  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<NodeFunction>::match(a, b);
    return end;
  }
};

//------------------------------------------------------------------------------

struct NodeConstructor : public NodeMaker<NodeConstructor> {
  using pattern = Seq<
    NodeDeclaredType,
    NodeParamList,
    Oneof<
      Atom<';'>,
      Ref<parse_statement_compound>
    >
  >;

  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<NodeConstructor>::match(a, b);
    return end;
  }
};

//------------------------------------------------------------------------------

struct NodeVariable : public NodeMaker<NodeVariable> {
  using pattern = Seq<
    NodeQualifiers,
    NodeSpecifier,
    Opt<comma_separated<
      Seq<
        NodeDeclarator,
        Opt<NodeBitSuffix>,
        Opt<
          Seq<
            Atom<'='>,
            NodeInitializer
          >
        >
      >
    >>
  >;

  // We specialize match() to dig out typedef'd identifiers
  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<NodeVariable>::match(a, b);
    if (end) {
      // Poke through the node on the top of the stack to find identifiers
      auto n1 = NodeBase::node_stack.back();
      if (!n1) return end;
      if (!n1->search<NodeKeyword<"typedef">>()) {
        return end;
      }

      if (auto n2 = n1->child<NodeDeclarator>()) {
        if (auto n3 = n2->child<NodeIdentifier>()) {
          auto s = n3->tok_a->lex->text();
          NodeBase::add_declared_type(s);
        }
      }
      return end;
    }
    else {
      return nullptr;
    }
  }
};

const Token* parse_declaration(const Token* a, const Token* b) {
  return NodeVariable::match(a, b);
}

//------------------------------------------------------------------------------

struct externalDeclaration : public PatternWrapper<externalDeclaration> {
  using pattern =
  Oneof<
    NodeFunction,
    Seq<NodeStruct, Atom<';'>>,
    Seq<NodeUnion, Atom<';'>>,
    Seq<NodeTemplate, Atom<';'>>,
    Seq<NodeClass, Atom<';'>>,
    Seq<NodeEnum, Atom<';'>>,
    Seq<NodeVariable, Atom<';'>>
  >;
};

const Token* parse_external_declaration(const Token* a, const Token* b) {
  return externalDeclaration::match(a, b);
}

//------------------------------------------------------------------------------
