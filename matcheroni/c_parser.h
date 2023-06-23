// CONTEXT
// TOP TYPEID

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
using comma_separated = Seq<P, Any<Seq<Atom<','>, P>>, Opt<Atom<','>> >;

template<typename P>
using opt_comma_separated = Opt<comma_separated<P>>;

//------------------------------------------------------------------------------

template<StringParam lit>
struct Operator {

  static Token* match(void* ctx, Token* a, Token* b) {
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
  using pattern = Oneof<
    Atom<LEX_FLOAT>,
    Atom<LEX_INT>,
    Atom<LEX_CHAR>,
    Atom<LEX_STRING>
  >;
};

//------------------------------------------------------------------------------

struct NodeBuiltinTypeBase {
  static Token* match(void* ctx, Token* a, Token* b) {
    if (a && ParseNode::builtin_types.contains(a->lex->text())) return a + 1;
    return nullptr;
  }
};

struct NodeBuiltinTypePrefix {
  static Token* match(void* ctx, Token* a, Token* b) {
    if (a && ParseNode::builtin_type_prefixes.contains(a->lex->text())) return a + 1;
    return nullptr;
  }
};

struct NodeBuiltinTypeSuffix {
  static Token* match(void* ctx, Token* a, Token* b) {
    if (a && ParseNode::builtin_type_suffixes.contains(a->lex->text())) return a + 1;
    return nullptr;
  }
};

//------------------------------------------------------------------------------
// Our builtin types are any sequence of prefixes followed by a builtin type

struct NodeBuiltinType : public NodeMaker<NodeBuiltinType> {
  using pattern = Seq<
    Any<
      Seq<NodeBuiltinTypePrefix, And<NodeBuiltinTypeBase>>
    >,
    NodeBuiltinTypeBase,
    Opt<NodeBuiltinTypeSuffix>
  >;
};

struct NodeClassType : public NodeMaker<NodeClassType> {
  static Token* match(void* ctx, Token* a, Token* b) {
    if (a && ParseNode::class_types.contains(a->lex->text())) return a + 1;
    return nullptr;
  }
};

struct NodeStructType : public NodeMaker<NodeStructType> {
  static Token* match(void* ctx, Token* a, Token* b) {
    if (a && ParseNode::struct_types.contains(a->lex->text())) return a + 1;
    return nullptr;
  }
};

struct NodeUnionType : public NodeMaker<NodeUnionType> {
  static Token* match(void* ctx, Token* a, Token* b) {
    if (a && ParseNode::union_types.contains(a->lex->text())) return a + 1;
    return nullptr;
  }
};

struct NodeEnumType : public NodeMaker<NodeEnumType> {
  static Token* match(void* ctx, Token* a, Token* b) {
    if (a && ParseNode::enum_types.contains(a->lex->text())) return a + 1;
    return nullptr;
  }
};

struct NodeTypedefType : public NodeMaker<NodeTypedefType> {
  static Token* match(void* ctx, Token* a, Token* b) {
    if (a && ParseNode::typedef_types.contains(a->lex->text())) return a + 1;
    return nullptr;
  }
};

//------------------------------------------------------------------------------
// (6.5.4) cast-expression:
//   unary-expression
//   ( type-name ) cast-expression

struct NodeExpressionCast : public NodeMaker<NodeExpressionCast> {
  using pattern = Seq<
    NodeOperator<"(">,
    NodeTypeName,
    NodeOperator<")">
  >;
};

//------------------------------------------------------------------------------

struct NodeExpressionParen : public NodeMaker<NodeExpressionParen> {
  using pattern = Seq<
    Atom<'('>,
    Opt<comma_separated<NodeExpression>>,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeExpressionBraces : public NodeMaker<NodeExpressionBraces> {
  using pattern = Seq<
    Atom<'{'>,
    Opt<comma_separated<NodeExpression>>,
    Atom<'}'>
  >;
};

//------------------------------------------------------------------------------

struct NodeExpressionCall : public NodeMaker<NodeExpressionCall> {
  using pattern = Seq<
    NodeIdentifier,
    NodeExpressionParen
  >;
};

//------------------------------------------------------------------------------

struct NodeExpressionSubscript : public NodeMaker<NodeExpressionSubscript> {
  using pattern = Seq<
    Atom<'['>,
    comma_separated<NodeExpression>,
    Atom<']'>
  >;
};

//------------------------------------------------------------------------------

struct NodeGccCompoundExpression : public NodeMaker<NodeGccCompoundExpression> {
  using pattern = Seq<
    Atom<'('>,
    NodeStatementCompound,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeExpressionTernary : public PatternWrapper<NodeExpressionTernary> {
  // pr68249.c - ternary option can be empty
  // pr49474.c - ternary branches can be comma-lists

  using pattern = Seq<
    NodeExpressionSoup,
    Opt<Seq<
      NodeOperator<"?">,
      Opt<comma_separated<NodeExpression>>,
      NodeOperator<":">,
      Opt<comma_separated<NodeExpression>>
    >>
  >;
};

//------------------------------------------------------------------------------

struct NodeExpression : public NodeMaker<NodeExpression> {
  using pattern = NodeExpressionTernary;
};

//------------------------------------------------------------------------------

struct NodeExpressionSoup : public PatternWrapper<NodeExpressionSoup> {
  constexpr inline static const char* op3 = "->*<<=<=>>>=";
  constexpr inline static const char* op2 = "---=->::!=.**=/=&&&=%=^=+++=<<<===>=>>|=||";
  constexpr inline static const char* op1 = "-!.*/&%^+<=>|~";

  static Token* match_operators(void* ctx, Token* a, Token* b) {
    if (b-a >= 3) {
      constexpr auto op_count = strlen(op3) / 3;
      for (auto j = 0; j < op_count; j++) {
        bool match = true;
        if (a->lex->span_a[0] != op3[j * 3 + 0]) match = false;
        if (a->lex->span_a[1] != op3[j * 3 + 1]) match = false;
        if (a->lex->span_a[2] != op3[j * 3 + 2]) match = false;
        if (match) return a + 3;
      }
    }

    if (b-a >= 2) {
      constexpr auto op_count = strlen(op2) / 2;
      for (auto j = 0; j < op_count; j++) {
        bool match = true;
        if (a->lex->span_a[0] != op2[j * 2 + 0]) match = false;
        if (a->lex->span_a[1] != op2[j * 2 + 1]) match = false;
        if (match) return a + 2;
      }
    }

    if (b-a >= 1) {
      constexpr auto op_count = strlen(op1) / 1;
      for (auto j = 0; j < op_count; j++) {
        bool match = true;
        if (a->lex->span_a[0] != op1[j * 1 + 0]) match = false;
        if (match) return a + 1;
      }
    }

    return nullptr;
  }

  using pattern = Seq<
    Some<
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
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeAttribute : public NodeMaker<NodeAttribute> {
  // 20010911-1.c - Attribute can be empty

  using pattern = Seq<
    Oneof<
      NodeKeyword<"__attribute__">,
      NodeKeyword<"__attribute">
    >,
    NodeOperator<"((">,
    Opt<comma_separated<NodeExpression>>,
    NodeOperator<"))">,
    Opt<NodeAttribute>
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

  static Token* match_qualifier(void* ctx, Token* a, Token* b) {
    if (!a || a == b) return nullptr;
    int qual_count = sizeof(qualifiers) / sizeof(qualifiers[0]);

    auto aa = a->lex->span_a;
    auto ab = a->lex->span_b;

    for (auto i = 0; i < qual_count; i++) {
      auto text = qualifiers[i];
      auto c = aa;
      for (; c < ab && (*c == *text) && *text; c++, text++);
      if (*text == 0) {
        return (c == a->lex->span_b) ? a + 1 : nullptr;
      }
    }

    return nullptr;
  }

  // This is the slowest matcher in the app, why?
  using pattern = Oneof<
    Seq<NodeKeyword<"_Alignas">, Atom<'('>, Oneof<NodeTypeDecl, NodeConstant>, Atom<')'>>,
    Seq<NodeKeyword<"__declspec">, Atom<'('>, NodeIdentifier, Atom<')'>>,
    NodeAttribute,
    Ref<match_qualifier>
  >;
};

//------------------------------------------------------------------------------

struct NodeQualifiers : public NodeMaker<NodeQualifiers> {
  using pattern = Some<NodeQualifier>;
};

//------------------------------------------------------------------------------

struct NodePointer : public NodeMaker<NodePointer> {
  using pattern =
  Seq<
    NodeOperator<"*">,
    Any<
      NodeOperator<"*">,
      NodeQualifier
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeParam : public NodeMaker<NodeParam> {
  using pattern = Oneof<
    Seq<
      Opt<NodeQualifiers>,
      NodeSpecifier,
      Opt<NodeQualifiers>,
      Opt<
        NodeDeclarator,
        NodeAbstractDeclarator
      >
    >,
    NodeIdentifier,
    NodeOperator<"...">
  >;
};

//------------------------------------------------------------------------------

struct NodeParamList : public NodeMaker<NodeParamList> {
  using pattern = Seq<
    Atom<'('>,
    Opt<comma_separated<NodeParam>>,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeArraySuffix : public NodeMaker<NodeArraySuffix> {
  using pattern = Oneof<
    Seq<Atom<'['>, Opt<NodeQualifiers>,                   Opt<NodeExpression>, Atom<']'>>,
    Seq<Atom<'['>, Keyword<"static">, Opt<NodeQualifiers>,    NodeExpression,  Atom<']'>>,
    Seq<Atom<'['>, Opt<NodeQualifiers>, Keyword<"static">,    NodeExpression,  Atom<']'>>,
    Seq<Atom<'['>, Opt<NodeQualifiers>, Atom<'*'>,                             Atom<']'>>
  >;
};

//------------------------------------------------------------------------------

struct NodeTemplateArgs : public NodeMaker<NodeTemplateArgs> {
  using pattern = Delimited<'<', '>',
    Opt<comma_separated<NodeExpression>>
  >;
};

//------------------------------------------------------------------------------

struct NodeAtomicType : public NodeMaker<NodeAtomicType> {
  using pattern = Seq<
    Keyword<"_Atomic">,
    Atom<'('>,
    NodeTypeDecl,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeSpecifier : public NodeMaker<NodeSpecifier> {
  using pattern = Seq<
    Oneof<
      Seq<Keyword<"class">,  NodeIdentifier>,
      Seq<Keyword<"union">,  NodeIdentifier>,
      Seq<Keyword<"struct">, NodeIdentifier>,
      Seq<Keyword<"enum">,   NodeIdentifier>,
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
      Seq<
        Oneof<
          Keyword<"__typeof__">,
          Keyword<"__typeof">,
          Keyword<"typeof">
        >,
        Atom<'('>,
        NodeExpression,
        Atom<')'>
      >
    >,
    Opt<NodeTemplateArgs>
  >;
};

//------------------------------------------------------------------------------
// (6.7.6) type-name:
//   specifier-qualifier-list abstract-declaratoropt

struct NodeTypeName : public NodeMaker<NodeTypeName> {
  using pattern = Seq<
    Some<
      NodeSpecifier,
      NodeQualifier
    >,
    Opt<NodeAbstractDeclarator>
  >;
};

//------------------------------------------------------------------------------
// Spec says the bit size can be any constant expression, but can we use just a
// constant or a paren-expression?

// (6.7.2.1) struct-declarator:
//   declarator
//   declaratoropt : constant-expression

struct NodeBitSuffix : public NodeMaker<NodeBitSuffix> {
  using pattern = Seq< Atom<':'>, NodeExpression >;
};

//------------------------------------------------------------------------------

struct NodeAsmSuffix : public NodeMaker<NodeAsmSuffix> {
  using pattern = Seq<
    Oneof<
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
  Seq<
    Opt<NodePointer>,
    Opt<Seq<Atom<'('>, NodeAbstractDeclarator, Atom<')'>>>,
    Any<
      NodeAttribute,
      NodeArraySuffix,
      NodeParamList
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeDeclarator : public NodeMaker<NodeDeclarator> {
  using pattern = Seq<
    Opt<NodeAttribute>,
    Opt<NodePointer>,
    Opt<NodeAttribute>,
    Opt<NodeQualifiers>,
    Oneof<
      NodeIdentifier,
      Seq<Atom<'('>, NodeDeclarator, Atom<')'>>
    >,
    Opt<NodeAsmSuffix>,
    Opt<NodeAttribute>,
    Opt<NodeBitSuffix>,
    Any<
      NodeAttribute,
      NodeArraySuffix,
      NodeParamList
    >
  >;
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

//------------------------------------------------------------------------------

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
    NodeDeclaration
  >;
};

struct NodeFieldList : public NodeMaker<NodeFieldList> {
  using pattern = Seq<
    Atom<'{'>,
    Any<
      Atom<';'>,
      NodeField
    >,
    Atom<'}'>
  >;
};

//------------------------------------------------------------------------------

struct NodeNamespace : public NodeMaker<NodeNamespace> {
  using pattern = Seq<
    Keyword<"namespace">,
    Opt<NodeIdentifier>,
    Opt<NodeFieldList>
  >;
};

//------------------------------------------------------------------------------

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
};

//------------------------------------------------------------------------------

struct NodeStructName : public NodeMaker<NodeStructName> {
  using pattern = Seq<
    Opt<NodeQualifiers>,
    Keyword<"struct">,
    Any<NodeAttribute>, // This has to be here, there are a lot of struct __attrib__() foo {};
    Opt<NodeIdentifier>
  >;

  // We specialize match() to dig out typedef'd identifiers
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = NodeMaker::match(ctx, a, b);
    if (end) {
      auto node = a->top;
      if (auto id = node->child<NodeIdentifier>()) {
        //printf("Adding struct type %s\n", id->text().c_str());
        ParseNode::struct_types.insert(id->text());
      }
    }
    return end;
  }
};

struct NodeStruct : public NodeMaker<NodeStruct> {
  using pattern = Seq<
    NodeStructName,
    Opt<NodeFieldList>,
    Any<NodeAttribute>,
    Opt<NodeDeclaratorList>
  >;
};

//------------------------------------------------------------------------------

struct NodeUnionName : public NodeMaker<NodeUnionName> {
  using pattern = Seq<
    Opt<NodeQualifiers>,
    Keyword<"union">,
    Any<NodeAttribute>,
    Opt<NodeIdentifier>
  >;

  // We specialize match() to dig out typedef'd identifiers
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = NodeMaker::match(ctx, a, b);
    if (end) {
      auto node = a->top;
      if (auto id = node->child<NodeIdentifier>()) {
        //printf("Adding union type %s\n", id->text().c_str());
        ParseNode::union_types.insert(id->text());
      }
    }
    return end;
  }
};


struct NodeUnion : public NodeMaker<NodeUnion> {
  using pattern = Seq<
    NodeUnionName,
    Opt<NodeFieldList>,
    Any<NodeAttribute>,
    Opt<NodeDeclaratorList>
  >;
};

//------------------------------------------------------------------------------

struct NodeClassName : public NodeMaker<NodeClassName> {
  using pattern = Seq<
    Opt<NodeQualifiers>,
    Keyword<"class">,
    Any<NodeAttribute>,
    Opt<NodeIdentifier>
  >;

  // We specialize match() to dig out typedef'd identifiers
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = NodeMaker::match(ctx, a, b);
    if (end) {
      auto node = a->top;
      if (auto id = node->child<NodeIdentifier>()) {
        //printf("Adding class type %s\n", id->text().c_str());
        ParseNode::class_types.insert(id->text());
      }
    }
    return end;
  }
};

struct NodeClass : public NodeMaker<NodeClass> {
  using pattern = Seq<
    NodeClassName,
    Opt<NodeFieldList>,
    Any<NodeAttribute>,
    Opt<NodeDeclaratorList>
  >;
};

//------------------------------------------------------------------------------

struct NodeTemplateParams : public NodeMaker<NodeTemplateParams> {
  using pattern = Delimited<'<', '>',
    comma_separated<NodeDeclaration>
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

struct NodeEnumName : public NodeMaker<NodeEnumName> {
  using pattern = Seq<
    Opt<NodeQualifiers>,
    Keyword<"enum">,
    Opt<Keyword<"class">>,
    Opt<NodeIdentifier>,
    Opt<Seq<Atom<':'>, NodeTypeDecl>>
  >;

  // We specialize match() to dig out typedef'd identifiers
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = NodeMaker::match(ctx, a, b);
    if (end) {
      auto node = a->top;
      if (auto id = node->child<NodeIdentifier>()) {
        //printf("Adding enum type %s\n", id->text().c_str());
        ParseNode::enum_types.insert(id->text());
      }
    }
    return end;
  }
};

struct NodeEnumerator : public NodeMaker<NodeEnumerator> {
  using pattern = Seq<
    NodeIdentifier,
    Opt<Seq<Atom<'='>, NodeExpression>>
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
    NodeEnumName,
    Opt<NodeEnumerators>,
    Opt<NodeDeclaratorList>
  >;
};

//------------------------------------------------------------------------------

struct NodeTypeDecl : public NodeMaker<NodeTypeDecl> {
  using pattern = Seq<
    Opt<NodeQualifiers>,
    NodeSpecifier,
    Opt<NodeAbstractDeclarator>
  >;
};

//------------------------------------------------------------------------------

struct NodeDesignation : public NodeMaker<NodeDesignation> {
  using pattern =
  Some<
    Seq<Atom<'['>, NodeConstant, Atom<']'>>,
    Seq<Atom<'.'>, NodeIdentifier>
  >;
};

struct NodeInitializerList : public PatternWrapper<NodeInitializerList> {
  using pattern = Seq<
    Atom<'{'>,
    opt_comma_separated<
      Seq<
        Opt<
          Seq<NodeDesignation, Atom<'='>>,
          Seq<NodeIdentifier,  Atom<':'>> // This isn't in the C grammar but compndlit-1.c uses it?
        >,
        NodeInitializer
      >
    >,
    //Opt<Atom<','>>,
    Atom<'}'>
  >;
};

struct NodeInitializer : public NodeMaker<NodeInitializer> {
  using pattern = Oneof<
    NodeInitializerList,
    NodeExpression
  >;
};

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
    Any<
      NodeQualifier,
      NodeAttribute,
      NodeSpecifier
    >,
    NodeFunctionIdentifier,
    NodeParamList,
    Opt<NodeAsmSuffix>,
    Opt<NodeKeyword<"const">>,
    Opt<Some<
      Seq<NodeDeclaration, Atom<';'>>
    >>,
    Oneof<
      Atom<';'>,
      NodeStatementCompound
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeConstructor : public NodeMaker<NodeConstructor> {
  using pattern = Seq<
    Oneof<
      NodeClassType,
      NodeStructType
    >,
    NodeParamList,
    Oneof<
      Atom<';'>,
      NodeStatementCompound
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeDeclaration : public NodeMaker<NodeDeclaration> {
  using pattern = Seq<
    // FIXME this is messy
    Opt<NodeAttribute>,
    Opt<NodeQualifiers>,
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
};

//------------------------------------------------------------------------------

struct NodeStatementCompound : public NodeMaker<NodeStatementCompound> {
  using pattern = Seq<
    Atom<'{'>,
    Any<NodeStatement>,
    Atom<'}'>
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementDeclaration : public NodeMaker<NodeStatementDeclaration> {
  using pattern = Seq<
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
  using pattern = Seq<
    Keyword<"for">,
    Atom<'('>,
    Opt<comma_separated<Oneof<
      NodeExpression,
      NodeDeclaration
    >>>,
    Atom<';'>,
    Opt<comma_separated<NodeExpression>>,
    Atom<';'>,
    Opt<comma_separated<NodeExpression>>,
    Atom<')'>,
    NodeStatement
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementElse : public NodeMaker<NodeStatementElse> {
  using pattern =
  Seq<
    Keyword<"else">,
    NodeStatement
  >;
};

struct NodeStatementIf : public NodeMaker<NodeStatementIf> {
  using pattern = Seq<
    Keyword<"if">,
    Seq<
      Atom<'('>,
      comma_separated<NodeExpression>,
      Atom<')'>
    >,
    NodeStatement,
    Opt<NodeStatementElse>
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementReturn : public NodeMaker<NodeStatementReturn> {
  using pattern = Seq<
    Keyword<"return">,
    NodeExpression,
    Atom<';'>
  >;
};


//------------------------------------------------------------------------------

struct NodeStatementCase : public NodeMaker<NodeStatementCase> {
  using pattern = Seq<
    Keyword<"case">,
    NodeExpression,
    Atom<':'>,
    Any<Seq<
      Not<Keyword<"case">>,
      Not<Keyword<"default">>,
      NodeStatement
    >>
  >;
};

struct NodeStatementDefault : public NodeMaker<NodeStatementDefault> {
  using pattern = Seq<
    Keyword<"default">,
    Atom<':'>,
    Any<Seq<
      Not<Keyword<"case">>,
      Not<Keyword<"default">>,
      NodeStatement
    >>
  >;
};

struct NodeStatementSwitch : public NodeMaker<NodeStatementSwitch> {
  using pattern = Seq<
    Keyword<"switch">,
    NodeExpression,
    Atom<'{'>,
    Any<
      NodeStatementCase,
      NodeStatementDefault
    >,
    Atom<'}'>
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementWhile : public NodeMaker<NodeStatementWhile> {
  using pattern = Seq<
    Keyword<"while">,
    Atom<'('>,
    comma_separated<NodeExpression>,
    Atom<')'>,
    NodeStatement
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementDoWhile : public NodeMaker<NodeStatementDoWhile> {
  using pattern = Seq<
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
  using pattern = Seq<
    NodeIdentifier,
    Atom<':'>,
    Opt<Atom<';'>>
  >;
};

//------------------------------------------------------------------------------

struct NodeAsmRef : public NodeMaker<NodeAsmRef> {
  using pattern = Seq<
    NodeString,
    Opt<Seq<
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
  Some<
    NodeKeyword<"volatile">,
    NodeKeyword<"__volatile">,
    NodeKeyword<"__volatile__">,
    NodeKeyword<"inline">,
    NodeKeyword<"goto">
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementAsm : public NodeMaker<NodeStatementAsm> {
  using pattern = Seq<
    Oneof<
      Keyword<"asm">,
      Keyword<"__asm">,
      Keyword<"__asm__">
    >,
    //NodeQualifiers,
    Opt<NodeAsmQualifiers>,
    Atom<'('>,
    NodeString, // assembly code
    Atom<':'>,
    Opt<NodeAsmRefs>, // output operands
    Opt<Seq<
      Atom<':'>,
      Opt<NodeAsmRefs>, // input operands
      Opt<Seq<
        Atom<':'>,
        Opt<NodeAsmRefs>, // clobbers
        Opt<Seq<
          Atom<':'>,
          Opt<comma_separated<NodeIdentifier>> // GotoLabels
        >>
      >>
    >>,
    Atom<')'>,
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementTypedef : public NodeMaker<NodeStatementTypedef> {
  using pattern = Seq<
    Opt<Keyword<"__extension__">>,
    Keyword<"typedef">,
    Oneof<
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

  static void extract_type(Token* a, Token* b) {
    auto node = a->top;

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

  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = NodeMaker::match(ctx, a, b);
    if (end) extract_type(a, b);
    return end;
  }
};

//------------------------------------------------------------------------------

// pr21356.c - Spec says goto should be an identifier, GCC allows expressions
struct NodeStatementGoto : public NodeMaker<NodeStatementGoto> {
  using pattern = Seq<
    Keyword<"goto">,
    NodeExpression
  >;
};

//------------------------------------------------------------------------------

struct NodeStatement : public PatternWrapper<NodeStatement> {
  using pattern = Oneof<
    // All of these have keywords first
    Seq<NodeClass,  Atom<';'>>,
    Seq<NodeStruct, Atom<';'>>,
    Seq<NodeUnion,  Atom<';'>>,
    Seq<NodeEnum,   Atom<';'>>,

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
    Seq<NodeStatementExpression,  Atom<';'>>,

    // Extra semicolons
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

class C99Parser {
public:
  C99Parser();
  void reset();

  void load(const std::string& path);
  void lex();
  bool parse();

  void dump_stats();
  void dump_lexemes();
  void dump_tokens();

  std::string text;

  std::vector<Lexeme> lexemes;
  std::vector<Token>  tokens;

  int file_total = 0;
  int file_pass = 0;
  int file_keep = 0;
  int file_bytes = 0;
  int file_lines = 0;

  double io_accum = 0;
  double lex_accum = 0;
  double parse_accum = 0;
  double cleanup_accum = 0;
};

//------------------------------------------------------------------------------
