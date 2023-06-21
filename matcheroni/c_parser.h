#pragma once

#include "ParseNode.h"
#include <string.h>

struct NodeAbstractDeclarator;
struct NodeClass;
struct NodeConstructor;
struct NodeDeclaration;
struct NodeDeclarator;
struct NodeEnum;
struct NodeExpression;
struct NodeExpressionSoup;
struct NodeFunction;
struct NodeInitializer;
struct NodeInitializerList;
struct NodeQualifier;
struct NodeSpecifier;
struct NodeStatement;
struct NodeStatementCompound;
struct NodeStatementTypedef;
struct NodeStruct;
struct NodeTemplate;
struct NodeTypeDecl;
struct NodeTypeName;
struct NodeUnion;

const char* match_text(const char** lits, int lit_count, const char* a, const char* b);

//------------------------------------------------------------------------------

template<typename P>
using comma_separated = Seq2<P, Any<Seq2<Atom<','>, P>>, Opt2<Atom<','>> >;

template<typename P>
using opt_comma_separated = Opt2<comma_separated<P>>;

//------------------------------------------------------------------------------

template<StringParam lit>
struct Operator {

  static const Token* match(const Token* a, const Token* b) {
    if (!a || a == b) return nullptr;
    if (a + sizeof(lit.value) > b) return nullptr;

    for (auto i = 0; i < lit.len; i++) {
      if (!a[i].is_punct(lit.value[i])) return nullptr;
    }

    return a + sizeof(lit.value);
  }
};

//------------------------------------------------------------------------------

template<StringParam lit>
struct NodeOperator : public NodeMaker<NodeOperator<lit>> {
  using pattern = Operator<lit>;
};

//------------------------------------------------------------------------------

template<StringParam lit>
struct NodeKeyword : public NodeMaker<NodeKeyword<lit>> {
  using pattern = Keyword<lit>;
};

//------------------------------------------------------------------------------

struct NodePreproc : public NodeMaker<NodePreproc> {
  using pattern = Atom<LEX_PREPROC>;
};

//------------------------------------------------------------------------------

struct NodeIdentifier : public NodeMaker<NodeIdentifier> {
  using pattern = Atom<LEX_IDENTIFIER>;

  std::string text() const {
    return tok_a->lex->text();
  }
};

//------------------------------------------------------------------------------

struct NodeString : public NodeMaker<NodeString> {
  using pattern = Atom<LEX_STRING>;
};

//------------------------------------------------------------------------------

struct NodeConstant : public NodeMaker<NodeConstant> {
  using pattern = Oneof2<
    Atom<LEX_FLOAT>,
    Atom<LEX_INT>,
    Atom<LEX_CHAR>,
    Atom<LEX_STRING>
  >;
};

//------------------------------------------------------------------------------

struct NodeBuiltinTypeBase {
  static const Token* match(const Token* a, const Token* b) {
    if (a && ParseNode::builtin_types.contains(a->lex->text())) return a + 1;
    return nullptr;
  }
};

struct NodeBuiltinTypePrefix {
  static const Token* match(const Token* a, const Token* b) {
    if (a && ParseNode::builtin_type_prefixes.contains(a->lex->text())) return a + 1;
    return nullptr;
  }
};

struct NodeBuiltinTypeSuffix {
  static const Token* match(const Token* a, const Token* b) {
    if (a && ParseNode::builtin_type_suffixes.contains(a->lex->text())) return a + 1;
    return nullptr;
  }
};

//------------------------------------------------------------------------------
// Our builtin types are any sequence of prefixes followed by a builtin type

struct NodeBuiltinType : public NodeMaker<NodeBuiltinType> {
  using pattern = Seq2<
    Any<
      Seq2<NodeBuiltinTypePrefix, And<NodeBuiltinTypeBase>>
    >,
    NodeBuiltinTypeBase,
    Opt2<NodeBuiltinTypeSuffix>
  >;
};

struct NodeClassType : public NodeMaker<NodeClassType> {
  static const Token* match(const Token* a, const Token* b) {
    if (a && ParseNode::class_types.contains(a->lex->text())) return a + 1;
    return nullptr;
  }
};

struct NodeStructType : public NodeMaker<NodeStructType> {
  static const Token* match(const Token* a, const Token* b) {
    if (a && ParseNode::struct_types.contains(a->lex->text())) return a + 1;
    return nullptr;
  }
};

struct NodeUnionType : public NodeMaker<NodeUnionType> {
  static const Token* match(const Token* a, const Token* b) {
    if (a && ParseNode::union_types.contains(a->lex->text())) return a + 1;
    return nullptr;
  }
};

struct NodeEnumType : public NodeMaker<NodeEnumType> {
  static const Token* match(const Token* a, const Token* b) {
    if (a && ParseNode::enum_types.contains(a->lex->text())) return a + 1;
    return nullptr;
  }
};

struct NodeTypedefType : public PatternWrapper<NodeTypedefType> {
  static const Token* match(const Token* a, const Token* b) {
    if (a && ParseNode::typedef_types.contains(a->lex->text())) return a + 1;
    return nullptr;
  }
};

//------------------------------------------------------------------------------
// (6.5.4) cast-expression:
//   unary-expression
//   ( type-name ) cast-expression

struct NodeExpressionCast : public NodeMaker<NodeExpressionCast> {
  using pattern = Seq2<
    Atom<'('>,
    NodeTypeName,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeExpressionParen : public NodeMaker<NodeExpressionParen> {
  using pattern = Seq2<
    Atom<'('>,
    Opt2<comma_separated<NodeExpression>>,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeExpressionBraces : public NodeMaker<NodeExpressionBraces> {
  using pattern = Seq2<
    Atom<'{'>,
    Opt2<comma_separated<NodeExpression>>,
    Atom<'}'>
  >;
};

//------------------------------------------------------------------------------

struct NodeExpressionCall : public NodeMaker<NodeExpressionCall> {
  using pattern = Seq2<
    NodeIdentifier,
    NodeExpressionParen
  >;
};

//------------------------------------------------------------------------------

struct NodeExpressionSubscript : public NodeMaker<NodeExpressionSubscript> {
  using pattern = Seq2<
    Atom<'['>,
    comma_separated<NodeExpression>,
    Atom<']'>
  >;
};

//------------------------------------------------------------------------------

struct NodeGccCompoundExpression : public NodeMaker<NodeGccCompoundExpression> {
  using pattern = Seq2<
    Atom<'('>,
    NodeStatementCompound,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeExpressionTernary : public PatternWrapper<NodeExpressionTernary> {
  // pr68249.c - ternary option can be empty
  // pr49474.c - ternary branches can be comma-lists

  using pattern = Seq2<
    NodeExpressionSoup,
    Opt2<Seq2<
      NodeOperator<"?">,
      Opt2<comma_separated<NodeExpression>>,
      NodeOperator<":">,
      Opt2<comma_separated<NodeExpression>>
    >>
  >;
};

//------------------------------------------------------------------------------

struct NodeExpression : public NodeMaker<NodeExpression> {
  using pattern = NodeExpressionTernary;
};

//------------------------------------------------------------------------------

struct NodeExpressionSoup : public PatternWrapper<NodeExpressionSoup> {
  inline static const char* all_operators[] = {
    "->*", "<<=", "<=>", ">>=", "--", "-=", "->", "::", "!=", ".*", "*=", "/=",
    "&&", "&=", "%=", "^=", "++", "+=", "<<", "<=", "==", ">=", ">>", "|=",
    "||", "-", "!", ".", "*", "/", "&", "%", "^", "+", "<", "=", ">", "|", "~",
  };

  static const Token* match_operator(const char* lit, const Token* a, const Token* b) {
    auto len = strlen(lit);
    if (!a || a == b) return nullptr;
    if (a + len > b) return nullptr;

    for (auto i = 0; i < len; i++) {
      if (!a[i].is_punct(lit[i])) return nullptr;
    }

    return a + len;
  }

  static const Token* match_operators(const Token* a, const Token* b) {
    if (!a || a == b) return nullptr;

    int op_count = sizeof(all_operators) / sizeof(all_operators[0]);
    for (int i = 0; i < op_count; i++) {
      if (auto end = match_operator(all_operators[i], a, b)) {
        return end;
      }
    }

    return nullptr;
  }

  using pattern = Seq2<
    Some<Oneof2<
      NodeGccCompoundExpression,
      NodeExpressionCall,
      NodeExpressionCast, // must be before NodeExpressionParen
      NodeExpressionParen,
      NodeInitializerList, // must be before NodeExpressionBraces
      NodeExpressionBraces,
      NodeExpressionSubscript,
      NodeKeyword<"sizeof">,
      NodeIdentifier,
      NodeConstant,
      Ref<NodeExpressionSoup::match_operators>
    >>
  >;
};

//------------------------------------------------------------------------------

struct NodeAttribute : public NodeMaker<NodeAttribute> {
  // 20010911-1.c - Attribute can be empty

  using pattern = Seq2<
    Oneof2<
      NodeKeyword<"__attribute__">,
      NodeKeyword<"__attribute">
    >,
    NodeOperator<"((">,
    Opt2<comma_separated<NodeExpression>>,
    NodeOperator<"))">,
    Opt2<NodeAttribute>
  >;
};

//------------------------------------------------------------------------------

struct NodeQualifier : public PatternWrapper<NodeQualifier> {
  inline static const char* qualifiers[] = {
    "__const", "__extension__", "__inline__", "__inline", "__restrict__",
    "__restrict", "__stdcall", "__volatile__", "__volatile", "_Noreturn",
    "_Thread_local", "auto", "const", "consteval", "constexpr", "constinit",
    "explicit", "extern", "inline", "mutable", "register", "restrict",
    "static", "thread_local", "__thread", /*"typedef",*/ "virtual", "volatile",
  };

  static const Token* match_qualifier(const Token* a, const Token* b) {
    if (!a || a == b) return nullptr;
    int qual_count = sizeof(qualifiers) / sizeof(qualifiers[0]);

    auto end = match_text(qualifiers, qual_count, a->lex->span_a, a->lex->span_b);
    return (end == a->lex->span_b) ? a + 1 : nullptr;
  }

  // This is the slowest matcher in the app, why?
  using pattern = Oneof2<
    CleanDeadNodes<Seq2<NodeKeyword<"_Alignas">, Atom<'('>, Oneof2<NodeTypeDecl, NodeConstant>, Atom<')'>>>,
    CleanDeadNodes<Seq2<NodeKeyword<"__declspec">, Atom<'('>, NodeIdentifier, Atom<')'>>>,
    CleanDeadNodes<NodeAttribute>,
    CleanDeadNodes<Ref<match_qualifier>>
  >;
};

//------------------------------------------------------------------------------

struct NodeQualifiers : public NodeMaker<NodeQualifiers> {
  using pattern = Some<NodeQualifier>;
};

//------------------------------------------------------------------------------

struct NodePointer : public NodeMaker<NodePointer> {
  using pattern =
  Seq2<
    NodeOperator<"*">,
    Any<Oneof2<
      NodeOperator<"*">,
      NodeQualifier
    >>
  >;
};

//------------------------------------------------------------------------------

struct NodeParam : public NodeMaker<NodeParam> {
  using pattern = Oneof2<
    CleanDeadNodes<Seq2<
      Opt2<NodeQualifiers>,
      NodeSpecifier,
      Opt2<NodeQualifiers>,
      Opt2<Oneof2<
        NodeDeclarator,
        NodeAbstractDeclarator
      >>
    >>,
    NodeIdentifier,
    NodeOperator<"...">
  >;
};

//------------------------------------------------------------------------------

struct NodeParamList : public NodeMaker<NodeParamList> {
  using pattern = Seq2<
    Atom<'('>,
    Opt2<comma_separated<NodeParam>>,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeArraySuffix : public NodeMaker<NodeArraySuffix> {
  using pattern = Oneof2<
    CleanDeadNodes<Seq2<Atom<'['>, Opt2<NodeQualifiers>,                   Opt2<NodeExpression>, Atom<']'>>>,
    CleanDeadNodes<Seq2<Atom<'['>, Keyword<"static">, Opt2<NodeQualifiers>,    NodeExpression,  Atom<']'>>>,
    CleanDeadNodes<Seq2<Atom<'['>, Opt2<NodeQualifiers>, Keyword<"static">,    NodeExpression,  Atom<']'>>>,
    CleanDeadNodes<Seq2<Atom<'['>, Opt2<NodeQualifiers>, Atom<'*'>,                             Atom<']'>>>
  >;
};

//------------------------------------------------------------------------------

struct NodeTemplateArgs : public NodeMaker<NodeTemplateArgs> {
  using pattern = Delimited<'<', '>',
    Opt2<comma_separated<NodeExpression>>
  >;
};

//------------------------------------------------------------------------------

struct NodeAtomicType : public NodeMaker<NodeAtomicType> {
  using pattern = Seq2<
    Keyword<"_Atomic">,
    Atom<'('>,
    NodeTypeDecl,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeSpecifier : public NodeMaker<NodeSpecifier> {
  using pattern = Seq2<
    Oneof2<
      Seq2<Keyword<"class">,  NodeIdentifier>,
      Seq2<Keyword<"union">,  NodeIdentifier>,
      Seq2<Keyword<"struct">, NodeIdentifier>,
      Seq2<Keyword<"enum">,   NodeIdentifier>,
      NodeBuiltinType,
      NodeTypedefType,
      /*
      // If this was C++, we would need to match these directly
      NodeClassType,
      NodeStructType,
      NodeUnionType,
      NodeEnumType,
      */
      NodeAtomicType,
      Seq2<
        Oneof2<
          Keyword<"__typeof__">,
          Keyword<"__typeof">,
          Keyword<"typeof">
        >,
        Atom<'('>,
        NodeExpression,
        Atom<')'>
      >
    >,
    Opt2<NodeTemplateArgs>
  >;
};

//------------------------------------------------------------------------------
// (6.7.6) type-name:
//   specifier-qualifier-list abstract-declaratoropt

struct NodeTypeName : public NodeMaker<NodeTypeName> {
  using pattern = Seq2<
    Some<Oneof2<
      NodeSpecifier,
      NodeQualifier
    >>,
    Opt2<NodeAbstractDeclarator>
  >;
};

//------------------------------------------------------------------------------
// Spec says the bit size can be any constant expression, but can we use just a
// constant or a paren-expression?

// (6.7.2.1) struct-declarator:
//   declarator
//   declaratoropt : constant-expression

struct NodeBitSuffix : public NodeMaker<NodeBitSuffix> {
  using pattern = Seq2< Atom<':'>, NodeExpression >;
};

//------------------------------------------------------------------------------

struct NodeAsmSuffix : public NodeMaker<NodeAsmSuffix> {
  using pattern = Seq2<
    Oneof2<
      Keyword<"asm">,
      Keyword<"__asm">,
      Keyword<"__asm__">
    >,
    Atom<'('>,
    Some<NodeString>,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeAbstractDeclarator : public NodeMaker<NodeAbstractDeclarator> {
  using pattern =
  Seq2<
    Opt2<NodePointer>,
    Opt2<Seq2<Atom<'('>, NodeAbstractDeclarator, Atom<')'>>>,
    Any<Oneof2<
      NodeAttribute,
      NodeArraySuffix,
      NodeParamList
    >>
  >;
};

//------------------------------------------------------------------------------

struct NodeDeclarator : public NodeMaker<NodeDeclarator> {
  using pattern = Seq2<
    Opt2<NodeAttribute>,
    Opt2<NodePointer>,
    Opt2<NodeAttribute>,
    Opt2<NodeQualifiers>,
    Oneof2<
      NodeIdentifier,
      Seq2<Atom<'('>, NodeDeclarator, Atom<')'>>
    >,
    Opt2<NodeAsmSuffix>,
    Opt2<NodeAttribute>,
    Opt2<NodeBitSuffix>,
    Any<Oneof2<
      NodeAttribute,
      NodeArraySuffix,
      NodeParamList
    >>
  >;
};

//------------------------------------------------------------------------------

struct NodeAccessSpecifier : public NodeMaker<NodeAccessSpecifier> {
  using pattern = Seq2<
    Oneof2<
      Keyword<"public">,
      Keyword<"private">
    >,
    Atom<':'>
  >;
};

//------------------------------------------------------------------------------

struct NodeField : public PatternWrapper<NodeField> {
  using pattern = Oneof2<
    NodeAccessSpecifier,
    NodeConstructor,
    NodeFunction,
    NodeStruct,
    NodeUnion,
    NodeTemplate,
    NodeClass,
    NodeEnum,
    NodeDeclaration
  >;
};

struct NodeFieldList : public NodeMaker<NodeFieldList> {
  using pattern = Seq2<
    Atom<'{'>,
    Any<Oneof2<
      Atom<';'>,
      NodeField
    >>,
    Atom<'}'>
  >;
};

//------------------------------------------------------------------------------

struct NodeNamespace : public NodeMaker<NodeNamespace> {
  using pattern = Seq2<
    Keyword<"namespace">,
    Opt2<NodeIdentifier>,
    Opt2<NodeFieldList>
  >;
};

//------------------------------------------------------------------------------

struct NodeDeclaratorList : public NodeMaker<NodeDeclaratorList> {
  using pattern =
  comma_separated<
    Seq2<
      Oneof2<
        Seq2<
          NodeDeclarator,
          Opt2<NodeBitSuffix>
        >,
        NodeBitSuffix
      >,
      Opt2<Seq2<
        Atom<'='>,
        NodeInitializer
      >>
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeStructName : public NodeMaker<NodeStructName> {
  using pattern = Seq2<
    Opt2<NodeQualifiers>,
    Keyword<"struct">,
    Any<NodeAttribute>, // This has to be here, there are a lot of struct __attrib__() foo {};
    Opt2<NodeIdentifier>
  >;

  // We specialize match() to dig out typedef'd identifiers
  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker::match(a, b);
    if (end) {
      auto node = ParseNode::stack_back();
      if (auto id = node->child<NodeIdentifier>()) {
        //printf("Adding struct type %s\n", id->text().c_str());
        ParseNode::struct_types.insert(id->text());
      }
    }
    return end;
  }
};

struct NodeStruct : public NodeMaker<NodeStruct> {
  using pattern = Seq2<
    NodeStructName,
    Opt2<NodeFieldList>,
    Any<NodeAttribute>,
    Opt2<NodeDeclaratorList>
  >;
};

//------------------------------------------------------------------------------

struct NodeUnionName : public NodeMaker<NodeUnionName> {
  using pattern = Seq2<
    Opt2<NodeQualifiers>,
    Keyword<"union">,
    Any<NodeAttribute>,
    Opt2<NodeIdentifier>
  >;

  // We specialize match() to dig out typedef'd identifiers
  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker::match(a, b);
    if (end) {
      auto node = ParseNode::stack_back();
      if (auto id = node->child<NodeIdentifier>()) {
        //printf("Adding union type %s\n", id->text().c_str());
        ParseNode::union_types.insert(id->text());
      }
    }
    return end;
  }
};


struct NodeUnion : public NodeMaker<NodeUnion> {
  using pattern = Seq2<
    NodeUnionName,
    Opt2<NodeFieldList>,
    Any<NodeAttribute>,
    Opt2<NodeDeclaratorList>
  >;
};

//------------------------------------------------------------------------------

struct NodeClassName : public NodeMaker<NodeClassName> {
  using pattern = Seq2<
    Opt2<NodeQualifiers>,
    Keyword<"class">,
    Any<NodeAttribute>,
    Opt2<NodeIdentifier>
  >;

  // We specialize match() to dig out typedef'd identifiers
  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker::match(a, b);
    if (end) {
      auto node = ParseNode::stack_back();
      if (auto id = node->child<NodeIdentifier>()) {
        //printf("Adding class type %s\n", id->text().c_str());
        ParseNode::class_types.insert(id->text());
      }
    }
    return end;
  }
};

struct NodeClass : public NodeMaker<NodeClass> {
  using pattern = Seq2<
    NodeClassName,
    Opt2<NodeFieldList>,
    Any<NodeAttribute>,
    Opt2<NodeDeclaratorList>
  >;
};

//------------------------------------------------------------------------------

struct NodeTemplateParams : public NodeMaker<NodeTemplateParams> {
  using pattern = Delimited<'<', '>',
    comma_separated<NodeDeclaration>
  >;
};

struct NodeTemplate : public NodeMaker<NodeTemplate> {
  using pattern = Seq2<
    Keyword<"template">,
    NodeTemplateParams,
    NodeClass
  >;
};

//------------------------------------------------------------------------------
// FIXME should probably have a few diffeerent versions instead of all the opts

struct NodeEnumName : public NodeMaker<NodeEnumName> {
  using pattern = Seq2<
    Opt2<NodeQualifiers>,
    Keyword<"enum">,
    Opt2<Keyword<"class">>,
    Opt2<NodeIdentifier>,
    Opt2<Seq2<Atom<':'>, NodeTypeDecl>>
  >;

  // We specialize match() to dig out typedef'd identifiers
  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker::match(a, b);
    if (end) {
      auto node = ParseNode::stack_back();
      if (auto id = node->child<NodeIdentifier>()) {
        //printf("Adding enum type %s\n", id->text().c_str());
        ParseNode::enum_types.insert(id->text());
      }
    }
    return end;
  }
};

struct NodeEnumerator : public NodeMaker<NodeEnumerator> {
  using pattern = Seq2<
    NodeIdentifier,
    Opt2<Seq2<Atom<'='>, NodeExpression>>
  >;
};

struct NodeEnumerators : public NodeMaker<NodeEnumerators> {
  using pattern = Seq2<
    Atom<'{'>,
    comma_separated<NodeEnumerator>,
    Opt2<Atom<','>>,
    Atom<'}'>
  >;
};

struct NodeEnum : public NodeMaker<NodeEnum> {
  using pattern = Seq2<
    NodeEnumName,
    Opt2<NodeEnumerators>,
    Opt2<NodeDeclaratorList>
  >;
};

//------------------------------------------------------------------------------

struct NodeTypeDecl : public NodeMaker<NodeTypeDecl> {
  using pattern = Seq2<
    Opt2<NodeQualifiers>,
    NodeSpecifier,
    Opt2<NodeAbstractDeclarator>
  >;
};

//------------------------------------------------------------------------------

struct NodeDesignation : public NodeMaker<NodeDesignation> {
  using pattern =
  Some<Oneof2<
    Seq2<Atom<'['>, NodeConstant, Atom<']'>>,
    Seq2<Atom<'.'>, NodeIdentifier>
  >>;
};

struct NodeInitializerList : public PatternWrapper<NodeInitializerList> {
  using pattern = Seq2<
    Atom<'{'>,
    opt_comma_separated<
      Seq2<
        Opt2<Oneof2<
          Seq2<NodeDesignation, Atom<'='>>,
          Seq2<NodeIdentifier,  Atom<':'>> // This isn't in the C grammar but compndlit-1.c uses it?
        >>,
        NodeInitializer
      >
    >,
    //Opt2<Atom<','>>,
    Atom<'}'>
  >;
};

struct NodeInitializer : public NodeMaker<NodeInitializer> {
  using pattern = Oneof2<
    NodeInitializerList,
    NodeExpression
  >;
};

//------------------------------------------------------------------------------

struct NodeFunctionIdentifier : public NodeMaker<NodeFunctionIdentifier> {
  using pattern = Seq2<
    Opt2<NodePointer>,
    Opt2<NodeAttribute>,
    Oneof2<
      NodeIdentifier,
      Seq2<Atom<'('>, NodeFunctionIdentifier, Atom<')'>>
    >
  >;
};


struct NodeFunction : public NodeMaker<NodeFunction> {
  using pattern = Seq2<
    Any<Oneof2<
      NodeQualifier,
      NodeAttribute,
      NodeSpecifier
    >>,
    NodeFunctionIdentifier,
    NodeParamList,
    Opt2<NodeAsmSuffix>,
    Opt2<NodeKeyword<"const">>,
    Opt2<Some<
      Seq2<NodeDeclaration, Atom<';'>>
    >>,
    Oneof2<
      Atom<';'>,
      NodeStatementCompound
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeConstructor : public NodeMaker<NodeConstructor> {
  using pattern = Seq2<
    Oneof2<
      NodeClassType,
      NodeStructType
    >,
    NodeParamList,
    Oneof2<
      Atom<';'>,
      NodeStatementCompound
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeDeclaration : public NodeMaker<NodeDeclaration> {
  using pattern = Seq2<
    // FIXME this is messy
    Opt2<NodeAttribute>,
    Opt2<NodeQualifiers>,
    Opt2<NodeAttribute>,

    // this is getting ridiculous
    Oneof2<
      Seq2<
        NodeSpecifier,
        Opt2<NodeAttribute>,
        Opt2<NodeQualifiers>,
        Opt2<NodeAttribute>,
        Opt2<NodeDeclaratorList>
      >,
      Seq2<
        Opt2<NodeSpecifier>,
        Opt2<NodeAttribute>,
        Opt2<NodeQualifiers>,
        Opt2<NodeAttribute>,
        NodeDeclaratorList
      >
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementCompound : public NodeMaker<NodeStatementCompound> {
  using pattern = Seq2<
    Atom<'{'>,
    Any<NodeStatement>,
    Atom<'}'>
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementDeclaration : public NodeMaker<NodeStatementDeclaration> {
  using pattern = Seq2<
    NodeDeclaration,
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementExpression : public NodeMaker<NodeStatementExpression> {
  using pattern = comma_separated<NodeExpression>;
};

//------------------------------------------------------------------------------

struct NodeStatementFor : public NodeMaker<NodeStatementFor> {
  using pattern = Seq2<
    Keyword<"for">,
    Atom<'('>,
    Opt2<comma_separated<Oneof2<
      NodeExpression,
      NodeDeclaration
    >>>,
    Atom<';'>,
    Opt2<comma_separated<NodeExpression>>,
    Atom<';'>,
    Opt2<comma_separated<NodeExpression>>,
    Atom<')'>,
    NodeStatement
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementElse : public NodeMaker<NodeStatementElse> {
  using pattern =
  Seq2<
    Keyword<"else">,
    NodeStatement
  >;
};

struct NodeStatementIf : public NodeMaker<NodeStatementIf> {
  using pattern = Seq2<
    Keyword<"if">,
    Seq2<
      Atom<'('>,
      comma_separated<NodeExpression>,
      Atom<')'>
    >,
    NodeStatement,
    Opt2<NodeStatementElse>
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementReturn : public NodeMaker<NodeStatementReturn> {
  using pattern = Seq2<
    Keyword<"return">,
    NodeExpression,
    Atom<';'>
  >;
};


//------------------------------------------------------------------------------

struct NodeStatementCase : public NodeMaker<NodeStatementCase> {
  using pattern = Seq2<
    Keyword<"case">,
    NodeExpression,
    Atom<':'>,
    Any<Seq2<
      Not<Keyword<"case">>,
      Not<Keyword<"default">>,
      NodeStatement
    >>
  >;
};

struct NodeStatementDefault : public NodeMaker<NodeStatementDefault> {
  using pattern = Seq2<
    Keyword<"default">,
    Atom<':'>,
    Any<Seq2<
      Not<Keyword<"case">>,
      Not<Keyword<"default">>,
      NodeStatement
    >>
  >;
};

struct NodeStatementSwitch : public NodeMaker<NodeStatementSwitch> {
  using pattern = Seq2<
    Keyword<"switch">,
    NodeExpression,
    Atom<'{'>,
    Any<Oneof2<
      NodeStatementCase,
      NodeStatementDefault
    >>,
    Atom<'}'>
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementWhile : public NodeMaker<NodeStatementWhile> {
  using pattern = Seq2<
    Keyword<"while">,
    Atom<'('>,
    comma_separated<NodeExpression>,
    Atom<')'>,
    NodeStatement
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementDoWhile : public NodeMaker<NodeStatementDoWhile> {
  using pattern = Seq2<
    Keyword<"do">,
    NodeStatement,
    Keyword<"while">,
    Atom<'('>,
    NodeExpression,
    Atom<')'>,
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementLabel: public NodeMaker<NodeStatementLabel> {
  using pattern = Seq2<
    NodeIdentifier,
    Atom<':'>,
    Opt2<Atom<';'>>
  >;
};

//------------------------------------------------------------------------------

struct NodeAsmRef : public NodeMaker<NodeAsmRef> {
  using pattern = Seq2<
    NodeString,
    Opt2<Seq2<
      Atom<'('>,
      NodeExpression,
      Atom<')'>
    >>
  >;
};

struct NodeAsmRefs : public NodeMaker<NodeAsmRefs> {
  using pattern = comma_separated<NodeAsmRef>;
};

//------------------------------------------------------------------------------

struct NodeAsmQualifiers : public NodeMaker<NodeAsmQualifiers> {
  using pattern =
  Some<Oneof2<
    NodeKeyword<"volatile">,
    NodeKeyword<"__volatile">,
    NodeKeyword<"__volatile__">,
    NodeKeyword<"inline">,
    NodeKeyword<"goto">
  >>;
};

//------------------------------------------------------------------------------

struct NodeStatementAsm : public NodeMaker<NodeStatementAsm> {
  using pattern = Seq2<
    Oneof2<
      Keyword<"asm">,
      Keyword<"__asm">,
      Keyword<"__asm__">
    >,
    //NodeQualifiers,
    Opt2<NodeAsmQualifiers>,
    Atom<'('>,
    NodeString, // assembly code
    Atom<':'>,
    Opt2<NodeAsmRefs>, // output operands
    Opt2<Seq2<
      Atom<':'>,
      Opt2<NodeAsmRefs>, // input operands
      Opt2<Seq2<
        Atom<':'>,
        Opt2<NodeAsmRefs>, // clobbers
        Opt2<Seq2<
          Atom<':'>,
          Opt2<comma_separated<NodeIdentifier>> // GotoLabels
        >>
      >>
    >>,
    Atom<')'>,
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementTypedef : public NodeMaker<NodeStatementTypedef> {
  using pattern = Seq2<
    Opt2<Keyword<"__extension__">>,
    Keyword<"typedef">,
    Oneof2<
      NodeStruct,
      NodeUnion,
      NodeClass,
      NodeEnum,
      NodeDeclaration
    >,
    Atom<';'>
  >;

  static void extract_declarator(const NodeDeclarator* decl) {
    if (auto id = decl->child<NodeIdentifier>()) {
      auto text = id->text();
      //printf("Adding typedef %s\n", text.c_str());
      ParseNode::typedef_types.insert(text);
    }

    for (auto child : decl) {
      if (auto decl = child->as<NodeDeclarator>()) {
        extract_declarator(decl);
      }
    }
  }

  static void extract_declarator_list(const NodeDeclaratorList* decls) {
    if (!decls) return;
    for (auto child : decls) {
      if (auto decl = child->as<NodeDeclarator>()) {
        extract_declarator(decl);
      }
    }
  }

  static void extract_type() {
    auto node = ParseNode::stack_back();
    if (!node) return;

    //node->dump_tree();

    if (auto type = node->child<NodeStruct>()) {
      extract_declarator_list(type->child<NodeDeclaratorList>());
      return;
    }

    if (auto type = node->child<NodeUnion>()) {
      extract_declarator_list(type->child<NodeDeclaratorList>());
      return;
    }

    if (auto type = node->child<NodeClass>()) {
      extract_declarator_list(type->child<NodeDeclaratorList>());
      return;
    }

    if (auto type = node->child<NodeEnum>()) {
      extract_declarator_list(type->child<NodeDeclaratorList>());
      return;
    }

    if (auto type = node->child<NodeDeclaration>()) {
      extract_declarator_list(type->child<NodeDeclaratorList>());
      return;
    }

    assert(false);
  }

  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker::match(a, b);
    if (end) extract_type();
    return end;
  }
};

//------------------------------------------------------------------------------

// pr21356.c - Spec says goto should be an identifier, GCC allows expressions
struct NodeStatementGoto : public NodeMaker<NodeStatementGoto> {
  using pattern = Seq2<
    Keyword<"goto">,
    NodeExpression
  >;
};

//------------------------------------------------------------------------------

struct NodeStatement : public PatternWrapper<NodeStatement> {
  using pattern = Oneof2<
    // All of these have keywords first
    Seq2<NodeClass,  Atom<';'>>,
    Seq2<NodeStruct, Atom<';'>>,
    Seq2<NodeUnion,  Atom<';'>>,
    Seq2<NodeEnum,   Atom<';'>>,

    NodeStatementTypedef,
    NodeStatementFor,
    NodeStatementIf,
    NodeStatementReturn,
    NodeStatementSwitch,
    NodeStatementDoWhile,
    NodeStatementWhile,
    NodeStatementAsm,

    // These don't - but they might confuse a keyword with an identifier...
    NodeStatementLabel,
    NodeStatementCompound,
    NodeFunction,
    NodeStatementDeclaration,
    Seq2<NodeStatementExpression,  Atom<';'>>,

    // Extra semicolons
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

template<typename P>
struct ProgressBar {
  static const Token* match(const Token* a, const Token* b) {
    printf("%.40s\n", a->lex->span_a);
    return P::match(a, b);
  }
};

struct NodeToplevelDeclaration : public PatternWrapper<NodeToplevelDeclaration> {
  using pattern =
  Oneof2<
    Atom<';'>,
    NodeFunction,
    NodeStatementTypedef,
    Seq2<NodeStruct,   Atom<';'>>,
    Seq2<NodeUnion,    Atom<';'>>,
    Seq2<NodeTemplate, Atom<';'>>,
    Seq2<NodeClass,    Atom<';'>>,
    Seq2<NodeEnum,     Atom<';'>>,
    Seq2<NodeDeclaration, Atom<';'>>
  >;
};

struct NodeTranslationUnit : public NodeMaker<NodeTranslationUnit> {
  using pattern = Any<
    //ProgressBar<
      Oneof2<
        NodePreproc,
        NodeToplevelDeclaration
      >
    //>
  >;
};

//------------------------------------------------------------------------------
