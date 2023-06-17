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
struct NodeDeclaratorList;

void extract_typedef();

//------------------------------------------------------------------------------

struct NodeAttribute : public NodeMaker<NodeAttribute> {
  using pattern = Seq<
    Oneof<
      NodeKeyword<"__attribute__">,
      NodeKeyword<"__attribute">
    >,
    NodeOperator<"((">,
    comma_separated<Ref<parse_expression>>,
    NodeOperator<"))">,
    Opt<NodeAttribute>
  >;
};

struct NodeQualifier : public PatternWrapper<NodeQualifier> {
  using pattern = Oneof<
    NodeKeyword<"__const">,
    NodeKeyword<"__extension__">,
    NodeKeyword<"__inline__">,
    NodeKeyword<"__inline">,
    NodeKeyword<"__restrict__">,
    NodeKeyword<"__restrict">,
    NodeKeyword<"__stdcall">,
    NodeKeyword<"__volatile__">,
    NodeKeyword<"__volatile">,
    NodeKeyword<"_Noreturn">,
    NodeKeyword<"_Thread_local">,
    NodeKeyword<"auto">,
    NodeKeyword<"const">,
    NodeKeyword<"consteval">,
    NodeKeyword<"constexpr">,
    NodeKeyword<"constinit">,
    NodeKeyword<"explicit">,
    NodeKeyword<"extern">,
    NodeKeyword<"inline">,
    NodeKeyword<"mutable">,
    NodeKeyword<"register">,
    NodeKeyword<"restrict">,
    NodeKeyword<"static">,
    NodeKeyword<"thread_local">,
    NodeKeyword<"typedef">,
    NodeKeyword<"virtual">,
    NodeKeyword<"volatile">,
    Seq<NodeKeyword<"_Alignas">, Atom<'('>, Oneof<NodeTypeDecl, NodeConstant>, Atom<')'>>,
    Seq<NodeKeyword<"__declspec">, Atom<'('>, NodeIdentifier, Atom<')'>>,
    NodeAttribute
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
      NodeQualifier
    >>
  >;
};

//------------------------------------------------------------------------------

/*
(6.7) declaration-specifiers:
  storage-class-specifier declaration-specifiersopt
  type-specifier declaration-specifiersopt
  type-qualifier declaration-specifiersopt
  function-specifier declaration-specifiersopt

(6.7.5) parameter-declaration:
  declaration-specifiers declarator
  declaration-specifiers abstract-declaratoropt
*/

struct NodeParam : public NodeMaker<NodeParam> {
  using pattern = Oneof<
    CleanDeadNodes<Seq<
      NodeQualifiers,
      NodeSpecifier,
      NodeQualifiers,
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
    Seq<Atom<'['>, NodeQualifiers,                   Opt<Ref<parse_expression>>, Atom<']'>>,
    Seq<Atom<'['>, Keyword<"static">, NodeQualifiers, Ref<parse_expression>, Atom<']'>>,
    Seq<Atom<'['>, NodeQualifiers, Keyword<"static">, Ref<parse_expression>, Atom<']'>>,
    Seq<Atom<'['>, NodeQualifiers, Atom<'*'>, Atom<']'>>
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
      Keyword<"struct">,
      Keyword<"enum">
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
// (6.7.6) type-name:
//   specifier-qualifier-list abstract-declaratoropt

struct NodeTypeName : public NodeMaker<NodeTypeName> {
  using pattern = Seq<
    Some<Oneof<
      NodeSpecifier,
      NodeQualifier
    >>,
    Opt<NodeAbstractDeclarator>
  >;
};

const Token* parse_type_name(const Token* a, const Token* b) {
  auto end = NodeTypeName::match(a, b);
  return end;
}

//------------------------------------------------------------------------------
// Spec says the bit size can be any constant expression, but can we use just a
// constant or a paren-expression?

// (6.7.2.1) struct-declarator:
//   declarator
//   declaratoropt : constant-expression

struct NodeBitSuffix : public NodeMaker<NodeBitSuffix> {
  using pattern =
  Seq<
    Atom<':'>,
    Oneof<
      NodeConstant,
      Seq<
        Atom<'('>,
        Ref<parse_expression>,
        Atom<')'>
      >
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeAbstractDeclarator : public NodeMaker<NodeAbstractDeclarator> {
  using pattern =
  Seq<
    Opt<NodePointer>,
    Opt<Seq<Atom<'('>, NodeAbstractDeclarator, Atom<')'>>>,
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
    Opt<NodeAttribute>,
    Opt<NodePointer>,
    Opt<NodeAttribute>,
    NodeQualifiers,
    Oneof<
      NodeIdentifier,
      Seq<Atom<'('>, NodeDeclarator, Atom<')'>>
    >,
    Opt<NodeAttribute>,
    Opt<NodeBitSuffix>,
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

struct NodeDeclaratorList : public NodeMaker<NodeDeclaratorList> {
  using pattern =
  comma_separated<
    Seq<
      Oneof<
        Seq<
          NodeDeclarator,
          Opt<NodeBitSuffix>
        >,
        NodeBitSuffix
      >,
      Opt<Seq<
        Atom<'='>,
        NodeInitializer
      >>
    >
  >;

  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker::match(a, b);
    return end;
  }
};

struct NodeDeclBody : public PatternWrapper<NodeDeclBody> {
  using pattern = Seq<
    Any<NodeAttribute>,
    Oneof<
      Seq< LogTypename<NodeIdentifier>, Opt<NodeFieldList>, Opt<NodeAttribute>, Opt<NodeDeclaratorList> >,
      Seq<                                  NodeFieldList,  Opt<NodeAttribute>, Opt<NodeDeclaratorList> >
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
    auto end = NodeMaker::match(a, b);
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
    Opt<Seq<Atom<'='>, Ref<parse_expression>>>
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
    Opt<NodeDeclaratorList>
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
    //Opt<Atom<','>>,
    Atom<'}'>
  >;

  static const Token* match(const Token* a, const Token* b) {
    auto end = PatternWrapper::match(a, b);
    return end;
  }
};

struct NodeInitializer : public NodeMaker<NodeInitializer> {
  using pattern = Oneof<
    NodeInitializerList,
    Ref<parse_expression>
  >;

  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker::match(a, b);
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
    NodeFunctionIdentifier,

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

    /*
    | NodeFunction 'typedef void (*frob)();'
    |  | NodeQualifiers 'typedef'
    |  |  | NodeKeyword 'typedef'
    |  | NodeSpecifier 'void'
    |  |  | NodeBuiltinType 'void'
    |  | NodeFunctionIdentifier '(*frob)'
    |  |  | NodeFunctionIdentifier '*frob'
    |  |  |  | NodePointer '*'
    |  |  |  |  | NodeOperator '*'
    |  |  |  | NodeIdentifier 'frob'
    |  | NodeParamList '()'
    */

    if (end) {
      // Check for function pointer typedef
      // FIXME typedefs should really be in their own node type...

      auto node = NodeBase::node_stack.back();
      if (!node) return end;

      auto quals = node->child<NodeQualifiers>();
      if (!quals || !quals->search<NodeKeyword<"typedef">>()) return end;

      auto id1 = node->child<NodeFunctionIdentifier>();
      if (!id1) return end;

      auto id2 = id1->child<NodeFunctionIdentifier>();
      if (!id2) return end;

      auto id3 = id2->child<NodePointer>();
      if (!id3) return end;

      auto id4 = id3->next;
      if (!id4->isa<NodeIdentifier>()) return end;

      auto s = id4->tok_a->lex->text();
      NodeBase::add_declared_type(s);

    }

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

void extract_typedef() {
  // Poke through the node on the top of the stack to find identifiers
  auto node = NodeBase::node_stack.back();
  if (!node) return;

  auto quals = node->child<NodeQualifiers>();
  if (!quals || !quals->search<NodeKeyword<"typedef">>()) return;

  auto list = node->child<NodeDeclaratorList>();
  if (!list) return;

  for (auto decl = list->head; decl; decl = decl->next) {
    auto id = decl->child<NodeIdentifier>();
    if (!id) return;

    auto s = id->tok_a->lex->text();
    NodeBase::add_declared_type(s);
  }
}

//------------------------------------------------------------------------------

struct NodeVariable : public NodeMaker<NodeVariable> {
  using pattern = Seq<
    // FIXME this is messy
    Opt<NodeAttribute>,
    NodeQualifiers,
    Opt<NodeAttribute>,

    // this is getting ridiculous
    Oneof<
      Seq<
        NodeSpecifier,
        Opt<NodeAttribute>,
        Opt<NodeQualifiers>,
        Opt<NodeAttribute>,
        Opt<NodeDeclaratorList>
      >,
      Seq<
        Opt<NodeSpecifier>,
        Opt<NodeAttribute>,
        Opt<NodeQualifiers>,
        Opt<NodeAttribute>,
        NodeDeclaratorList
      >
    >
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
