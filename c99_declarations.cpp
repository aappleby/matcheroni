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
struct NodeStruct;
struct NodeUnion;
struct NodeClass;
struct NodeTemplate;

void extract_typedef();

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

struct NodeAttribute : public NodeMaker<NodeAttribute> {
  using pattern = Seq<
    NodeKeyword<"__attribute__">,
    NodeOperator<"((">,
    comma_separated<Ref<parse_expression>>,
    NodeOperator<"))">
  >;
};

struct NodeQualifier : public PatternWrapper<NodeQualifier> {
  using pattern = Oneof<
    NodeTypeQualifier,
    NodeAlignmentQualifier,
    NodeFunctionQualifier,
    NodeAttribute,
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
    NodeKeyword<"__restrict__">,
    NodeKeyword<"__extension__">
  >;
};

struct NodeQualifiers : public NodeMaker<NodeQualifiers> {
  using pattern = Any<NodeQualifier>;

  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<NodeQualifiers>::match(a, b);
    return end;
  }
};

const Token* parse_qualifiers(const Token* a, const Token* b) {
  return NodeQualifiers::match(a, b);
}

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
    CleanDeadNodes<Seq<
      NodeQualifiers,
      NodeSpecifier,
      Opt<Oneof<
        NodeDeclarator,
        NodeAbstractDeclarator
      >>
    >>,
    NodeIdentifier,
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
    Opt<Oneof<
      Keyword<"class">,
      Keyword<"union">,
      Keyword<"struct">
    >>,
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
  using pattern =
  Seq<
    Oneof<
      NodePointer,
      Seq<
        Opt<NodePointer>,
        Seq<Atom<'('>, NodeAbstractDeclarator, Atom<')'>>
      >,
      Seq<
        Opt<NodePointer>,
        Some<Oneof<
          NodeAttribute,
          NodeArraySuffix,
          NodeParamList
        >>
      >
    >,
    Any<Oneof<
      NodeAttribute,
      NodeArraySuffix,
      NodeParamList
    >>
  >;

  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<NodeAbstractDeclarator>::match(a, b);
    return end;
  }
};

//------------------------------------------------------------------------------

struct NodeDeclarator : public NodeMaker<NodeDeclarator> {
  using pattern = Seq<
    Oneof<
      Seq<
        Opt<NodePointer>,
        Opt<NodeAttribute>,
        NodeQualifiers,
        NodeIdentifier,
        Opt<NodeAttribute>,
        Opt<NodeBitSuffix>
      >,
      Seq<
        Opt<NodePointer>,
        Atom<'('>, NodeDeclarator, Atom<')'>
      >
    >,
    Any<Oneof<
      NodeAttribute,
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

void extract_typedef() {
  // Poke through the node on the top of the stack to find identifiers
  auto node = NodeBase::node_stack.back();
  if (!node) return;

  auto quals = node->child<NodeQualifiers>();
  if (!quals || !quals->search<NodeKeyword<"typedef">>()) return;

  auto decl = node->child<NodeDeclarator>();
  if (!decl) return;

  auto id = decl->child<NodeIdentifier>();
  if (!id) return;

  auto s = id->tok_a->lex->text();
  NodeBase::add_declared_type(s);
}

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
    NodeFunction,
    NodeStruct,
    NodeUnion,
    NodeTemplate,
    NodeClass,
    NodeEnum,
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
    Any<Oneof<
      Atom<';'>,
      NodeField
    >>,
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

// struct           ; // X
// struct        bar; // X
// struct     {}    ; // X
// struct     {} bar;
// struct foo       ;
// struct foo    bar;
// struct foo {}    ;
// struct foo {} bar;

/*
        Opt<Oneof<
          Seq<NodeDesignation, Atom<'='>>,
          Seq<NodeIdentifier,  Atom<':'>> // This isn't in the C99 grammar but compndlit-1.c uses it?
        >>,
*/


struct DeclThing : public PatternWrapper<DeclThing> {
  using pattern =
  comma_separated<
    Seq<
      NodeDeclarator,
      Opt<Seq<
        Atom<'='>,
        NodeInitializer
      >>
    >
  >;

  static const Token* match(const Token* a, const Token* b) {
    auto end = PatternWrapper::match(a, b);
    return end;
  }
};

struct NodeDeclBody : public PatternWrapper<NodeDeclBody> {
  using pattern = Seq<
    Any<NodeAttribute>,
    Oneof<
      Seq< LogTypename<NodeIdentifier>, Opt<NodeFieldList>, Opt<NodeAttribute>, Opt<DeclThing> >,
      Seq<                                  NodeFieldList,  Opt<NodeAttribute>,     DeclThing  >
    >
  >;
};

struct NodeStruct : public NodeMaker<NodeStruct> {
  using pattern = Seq<
    NodeQualifiers,
    Keyword<"struct">,
    NodeDeclBody
  >;

  // We specialize match() to dig out typedef'd identifiers
  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<NodeStruct>::match(a, b);
    if (end) extract_typedef();
    return end;
  }
};

struct NodeUnion : public NodeMaker<NodeUnion> {
  using pattern = Seq<
    NodeQualifiers,
    Keyword<"union">,
    NodeDeclBody
  >;

  // We specialize match() to dig out typedef'd identifiers
  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<NodeUnion>::match(a, b);
    if (end) extract_typedef();
    return end;
  }
};

struct NodeClass : public NodeMaker<NodeClass> {
  using pattern = Seq<
    NodeQualifiers,
    Keyword<"class">,
    NodeDeclBody
  >;

  // We specialize match() to dig out typedef'd identifiers
  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<NodeClass>::match(a, b);
    if (end) extract_typedef();
    return end;
  }
};

const Token* parse_class(const Token* a, const Token* b) {
  auto end = NodeClass::match(a, b);
  return end;
}

const Token* parse_struct(const Token* a, const Token* b) {
  auto end = NodeStruct::match(a, b);
  return end;
}

const Token* parse_union(const Token* a, const Token* b) {
  auto end = NodeUnion::match(a, b);
  return end;
}

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
    NodeQualifiers,
    Keyword<"enum">,
    Opt<Keyword<"class">>,
    Opt<LogTypename<NodeIdentifier>>,
    Opt<Seq<Atom<':'>, NodeTypeDecl>>,
    Opt<NodeEnumerators>,
    Opt<DeclThing>
  >;

  // We specialize match() to dig out typedef'd identifiers
  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<NodeEnum>::match(a, b);
    if (end) extract_typedef();
    return end;
  }
};

const Token* parse_enum(const Token* a, const Token* b) {
  return NodeEnum::match(a, b);
}

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
        Opt<Oneof<
          Seq<NodeDesignation, Atom<'='>>,
          Seq<NodeIdentifier,  Atom<':'>> // This isn't in the C99 grammar but compndlit-1.c uses it?
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

const Token* parse_initializer_list(const Token* a, const Token* b) {
  auto end = NodeInitializerList::match(a, b);
  return end;
}

//------------------------------------------------------------------------------

struct NodeFunctionIdentifier : public NodeMaker<NodeFunctionIdentifier> {
  using pattern = Seq<
    Opt<NodePointer>,
    Opt<NodeAttribute>,
    Oneof<
      NodeIdentifier,
      Seq<Atom<'('>, NodeFunctionIdentifier, Atom<')'>>
    >
  >;
};


struct NodeFunction : public NodeMaker<NodeFunction> {
  using pattern = Seq<
    NodeQualifiers,
    Opt<NodeAttribute>,
    Opt<NodeSpecifier>,
    Opt<NodeAttribute>,
    Opt<NodeFunctionIdentifier>,

    NodeParamList,
    Opt<NodeAttribute>,
    Opt<NodeKeyword<"const">>,
    Opt<Some<
      Seq<NodeVariable, Atom<';'>>
    >>,

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

const Token* parse_function(const Token* a, const Token* b) {
  auto end = NodeFunction::match(a, b);
  return end;
}

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
    // FIXME this is messy
    Opt<NodeAttribute>,
    NodeQualifiers,
    Opt<NodeAttribute>,
    NodeSpecifier,
    Opt<NodeAttribute>,
    NodeQualifiers,
    Opt<NodeAttribute>,

    Opt<comma_separated<
      Seq<
        Oneof<
          Seq<
            NodeDeclarator
          >,
          Seq<
            Opt<NodeDeclarator>,
            NodeBitSuffix
          >
        >,
        Opt<Seq<
          Atom<'='>,
          NodeInitializer
        >>
      >
    >>
  >;

  // We specialize match() to dig out typedef'd identifiers
  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<NodeVariable>::match(a, b);
    if (end) extract_typedef();
    return end;
  }
};

const Token* parse_declaration(const Token* a, const Token* b) {
  return NodeVariable::match(a, b);
}

//------------------------------------------------------------------------------

struct externalDeclaration : public PatternWrapper<externalDeclaration> {
  using pattern =
  Oneof<
    Atom<';'>,
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
  auto end = externalDeclaration::match(a, b);
  return end;
}

//------------------------------------------------------------------------------
