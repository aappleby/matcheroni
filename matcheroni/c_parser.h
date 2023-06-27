#pragma once

#include "ParseNode.h"
#include "c_constants.h"

void dump_tree(const ParseNode* n, int max_depth, int indentation);

struct NodeAbstractDeclarator;
struct NodeAttribute;
struct NodeClass;
struct NodeClassDecl;
struct NodeConstructor;
struct NodeDeclaration;
struct NodeDeclarator;
struct NodeEnum;
struct NodeExpression;
struct NodeFunction;
struct NodeIdentifier;
struct NodeInitializer;
struct NodeInitializerList;
struct NodeQualifier;
struct NodeQualifiers;
struct NodeSpecifier;
struct NodeStatement;
struct NodeStatementCompound;
struct NodeStatementTypedef;
struct NodeStruct;
struct NodeStructType;
struct NodeTemplate;
struct NodeTypeDecl;
struct NodeTypeName;
struct NodeUnion;

struct NodeClassType   : public NodeSpan {};
struct NodeUnionType   : public NodeSpan {};
struct NodeEnumType    : public NodeSpan {};
struct NodeTypedefType : public NodeSpan {};

//------------------------------------------------------------------------------

class C99Parser {
public:
  C99Parser();
  void reset();

  void load(const std::string& path);
  void lex();
  ParseNode* parse();

  Token* match_builtin_type_base  (Token* a, Token* b);
  Token* match_builtin_type_prefix(Token* a, Token* b);
  Token* match_builtin_type_suffix(Token* a, Token* b);

  //----------------------------------------------------------------------------

  void add_type(const std::string& t, std::vector<std::string>& types) {
    if (std::find(types.begin(), types.end(), t) == types.end()) {
      types.push_back(t);
    }
  }

  Token* match_type(const std::vector<std::string>& types, Token* a, Token* b) {
    if (!a || a == b) return nullptr;
    if (types.empty()) return nullptr;
    for (const auto& c : types) {
      auto r = cmp_span_lit(a->lex->span_a, a->lex->span_b, c.c_str());
      if (r == 0) return a + 1;
    }
    return nullptr;
  }

  //----------------------------------------------------------------------------

  void add_class_type  (const std::string& t) { add_type(t, class_types); }
  void add_struct_type (const std::string& t) { add_type(t, struct_types); }
  void add_union_type  (const std::string& t) { add_type(t, union_types); }
  void add_enum_type   (const std::string& t) { add_type(t, enum_types); }
  void add_typedef_type(const std::string& t) { add_type(t, typedef_types); }

  Token* match_class_type  (Token* a, Token* b) { return match_type(class_types,   a, b); }
  Token* match_struct_type (Token* a, Token* b) { return match_type(struct_types,  a, b); }
  Token* match_union_type  (Token* a, Token* b) { return match_type(union_types,   a, b); }
  Token* match_enum_type   (Token* a, Token* b) { return match_type(enum_types,    a, b); }
  Token* match_typedef_type(Token* a, Token* b) { return match_type(typedef_types, a, b); }

  void dump_stats();
  void dump_lexemes();
  void dump_tokens();

  std::string text;
  std::vector<Lexeme> lexemes;
  std::vector<Token>  tokens;

  ParseNode* root = nullptr;

  int file_pass = 0;
  int file_fail = 0;
  int file_skip = 0;
  int file_bytes = 0;
  int file_lines = 0;

  double io_accum = 0;
  double lex_accum = 0;
  double parse_accum = 0;
  double cleanup_accum = 0;

  std::vector<std::string> class_types;
  std::vector<std::string> struct_types;
  std::vector<std::string> union_types;
  std::vector<std::string> enum_types;
  std::vector<std::string> typedef_types;
};

//------------------------------------------------------------------------------
// Our builtin types are any sequence of prefixes followed by a builtin type

struct NodeBuiltinType : public NodeBaseMaker<NodeBuiltinType> {
  using match_base   = Ref<&C99Parser::match_builtin_type_base>;
  using match_prefix = Ref<&C99Parser::match_builtin_type_prefix>;
  using match_suffix = Ref<&C99Parser::match_builtin_type_suffix>;

  using pattern = Seq<
    Any<Seq<match_prefix, And<match_base>>>,
    match_base,
    Opt<match_suffix>
  >;
};

template<StringParam lit>
struct NodeOpPrefix : public NodeBase {
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = match_punct(ctx, a, b, lit.str_val, lit.str_len);
    if (end) {
      auto node = new NodeOpPrefix<lit>();
      node->precedence = prefix_precedence(lit.str_val);
      node->init_base(a, end - 1);
    }
    return end;
  }
};

template<StringParam lit>
struct NodeOpBinary : public NodeBase {
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = match_punct(ctx, a, b, lit.str_val, lit.str_len);
    if (end) {
      auto node = new NodeOpBinary<lit>();
      node->precedence = binary_precedence(lit.str_val);
      node->init_base(a, end - 1);
    }
    return end;
  }
};

template<StringParam lit>
struct NodeOpSuffix : public NodeBase {
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = match_punct(ctx, a, b, lit.str_val, lit.str_len);
    if (end) {
      auto node = new NodeOpSuffix<lit>();
      node->precedence = suffix_precedence(lit.str_val);
      node->init_base(a, end - 1);
    }
    return end;
  }
};

//------------------------------------------------------------------------------
// (6.5.4) cast-expression:
//   unary-expression
//   ( type-name ) cast-expression

struct NodeExpressionCast : public NodeSpanMaker<NodeExpressionCast> {
  using pattern = Seq<
    NodePunc<"(">,
    NodeTypeName,
    NodePunc<")">
  >;
};

//------------------------------------------------------------------------------

struct NodeExpressionParen : public NodeSpanMaker<NodeExpressionParen> {
  using pattern = Seq<
    NodePunc<"(">,
    Opt<comma_separated<NodeExpression>>,
    NodePunc<")">
  >;
};

//------------------------------------------------------------------------------

struct NodeExpressionBraces : public NodeSpanMaker<NodeExpressionBraces> {
  using pattern = Seq<
    NodePunc<"{">,
    Opt<comma_separated<NodeExpression>>,
    NodePunc<"}">
  >;
};

//------------------------------------------------------------------------------

/*
struct NodeExpressionCall : public NodeMaker<NodeExpressionCall> {
  using pattern = Seq<
    NodeIdentifier,
    NodeExpressionParen
  >;
};
*/

//------------------------------------------------------------------------------

struct NodeExpressionSubscript : public NodeSpanMaker<NodeExpressionSubscript> {
  using pattern = Seq<
    NodePunc<"[">,
    comma_separated<NodeExpression>,
    NodePunc<"]">
  >;
};

//------------------------------------------------------------------------------
// This is a weird ({...}) thing that GCC supports

struct NodeExpressionGccCompound : public NodeSpanMaker<NodeExpressionGccCompound> {
  using pattern = Seq<
    Opt<NodeKeyword<"__extension__">>,
    NodePunc<"({">,
    Any<NodeStatement>,
    NodePunc<"})">
  >;
};

//------------------------------------------------------------------------------
// This captures all expresion forms, but does _not_ bother to put them into a
// tree or do operator precedence or anything.

// FIXME - replace with some other parser

struct NodeExpressionTernary : public NodeSpan {};
struct NodeExpressionBinary  : public NodeSpan {};
struct NodeExpressionPrefix  : public NodeSpan {};
struct NodeExpressionCore    : public NodeSpan {};
struct NodeExpressionSuffix  : public NodeSpan {};

struct NodeExpressionSizeof  : public NodeSpanMaker<NodeExpressionSizeof> {
  using pattern = Seq<
    NodeKeyword<"sizeof">,
    Oneof<
      NodeExpressionCast,
      NodeExpressionParen,
      NodeExpression
    >
  >;
};

struct NodeExpressionAlignof  : public NodeSpanMaker<NodeExpressionAlignof> {
  using pattern = Seq<
    NodeKeyword<"__alignof__">,
    Oneof<
      NodeExpressionCast,
      NodeExpressionParen
    >
  >;
};

struct NodeExpressionOffsetof  : public NodeSpanMaker<NodeExpressionOffsetof> {
  using pattern = Seq<
    Oneof<
      NodeKeyword<"offsetof">,
      NodeKeyword<"__builtin_offsetof">
    >,
    NodePunc<"(">,
    NodeTypeName,
    NodePunc<",">,
    NodeExpression,
    NodePunc<")">
  >;
};

//----------------------------------------

using prefix_op =
Oneof<
  NodeExpressionCast,
  NodeKeyword<"__extension__">,
  NodeKeyword<"__real">,
  NodeKeyword<"__real__">,
  NodeKeyword<"__imag">,
  NodeKeyword<"__imag__">,
  NodeOpPrefix<"++">,
  NodeOpPrefix<"--">,
  NodeOpPrefix<"+">,
  NodeOpPrefix<"-">,
  NodeOpPrefix<"!">,
  NodeOpPrefix<"~">,
  NodeOpPrefix<"*">,
  NodeOpPrefix<"&">
>;

//----------------------------------------

using core = Oneof<
  NodeExpressionSizeof,
  NodeExpressionAlignof,
  NodeExpressionOffsetof,
  NodeExpressionGccCompound,
  NodeExpressionParen,
  NodeInitializerList,
  NodeExpressionBraces,
  NodeIdentifier,
  NodeConstant
>;

//----------------------------------------

using suffix_op =
Oneof<
  NodeInitializerList,   // must be before NodeExpressionBraces
  NodeExpressionBraces,
  NodeExpressionParen,
  NodeExpressionSubscript,
  NodeOpSuffix<"++">,
  NodeOpSuffix<"--">
>;

//----------------------------------------

using binary_op =
Oneof<
  NodeOpBinary<"<<=">,
  NodeOpBinary<">>=">,
  NodeOpBinary<"->*">,
  NodeOpBinary<"<=>">,
  NodeOpBinary<".*">,
  NodeOpBinary<"::">,
  NodeOpBinary<"-=">,
  NodeOpBinary<"->">,
  NodeOpBinary<"!=">,
  NodeOpBinary<"*=">,
  NodeOpBinary<"/=">,
  NodeOpBinary<"&=">,
  NodeOpBinary<"%=">,
  NodeOpBinary<"^=">,
  NodeOpBinary<"+=">,
  NodeOpBinary<"<<">,
  NodeOpBinary<"<=">,
  NodeOpBinary<"==">,
  NodeOpBinary<">=">,
  NodeOpBinary<">>">,
  NodeOpBinary<"|=">,
  NodeOpBinary<"||">,
  NodeOpBinary<"&&">,
  NodeOpBinary<".">,
  NodeOpBinary<"/">,
  NodeOpBinary<"&">,
  NodeOpBinary<"%">,
  NodeOpBinary<"^">,
  NodeOpBinary<"<">,
  NodeOpBinary<"=">,
  NodeOpBinary<">">,
  NodeOpBinary<"|">,
  NodeOpBinary<"-">,
  NodeOpBinary<"*">,
  NodeOpBinary<"+">
>;

struct NodeExpressionUnit : public NodeSpanMaker<NodeExpressionUnit> {
  using pattern = Seq<
    Any<prefix_op>,
    core,
    Any<suffix_op>
  >;
};


struct NodeExpression {


  /*
  // This mess only speeds up the CSmith test by 3%
  static Token* match_binary_op(void* ctx, Token* a, Token* b) {
    if (!a || a == b) return nullptr;
    switch(a->lex->span_a[0]) {
      case '!': return NodeOpBinary<"!=">::match(ctx, a, b);
      case '%': return Oneof< NodeOpBinary<"%=">, NodeOpBinary<"%"> >::match(ctx, a, b);
      case '&': return Oneof< NodeOpBinary<"&&">, NodeOpBinary<"&=">, NodeOpBinary<"&"> >::match(ctx, a, b);
      case '*': return Oneof< NodeOpBinary<"*=">, NodeOpBinary<"*"> >::match(ctx, a, b);
      case '+': return Oneof< NodeOpBinary<"+=">, NodeOpBinary<"+"> >::match(ctx, a, b);
      case '-': return Oneof< NodeOpBinary<"->*">, NodeOpBinary<"->">, NodeOpBinary<"-=">, NodeOpBinary<"-"> >::match(ctx, a, b);
      case '.': return Oneof< NodeOpBinary<".*">, NodeOpBinary<"."> >::match(ctx, a, b);
      case '/': return Oneof< NodeOpBinary<"/=">, NodeOpBinary<"/"> >::match(ctx, a, b);
      case ':': return Oneof< NodeOpBinary<"::"> >::match(ctx, a, b);
      case '<': return Oneof< NodeOpBinary<"<<=">, NodeOpBinary<"<=>">, NodeOpBinary<"<=">, NodeOpBinary<"<<">, NodeOpBinary<"<"> >::match(ctx, a, b);
      case '=': return Oneof< NodeOpBinary<"==">, NodeOpBinary<"="> >::match(ctx, a, b);
      case '>': return Oneof< NodeOpBinary<">>=">, NodeOpBinary<">=">, NodeOpBinary<">>">, NodeOpBinary<">"> >::match(ctx, a, b);
      case '^': return Oneof< NodeOpBinary<"^=">, NodeOpBinary<"^"> >::match(ctx, a, b);
      case '|': return Oneof< NodeOpBinary<"||">, NodeOpBinary<"|=">, NodeOpBinary<"|"> >::match(ctx, a, b);
      default:  return nullptr;
    }
  }
  */

  //----------------------------------------

  static Token* match2(void* ctx, Token* a, Token* b) {
    Token* cursor = a;

    using pattern = Seq<
      Any<prefix_op>,
      core,
      Any<suffix_op>
    >;
    cursor = NodeExpressionUnit::match(ctx, a, b);
    if (!cursor) return nullptr;

    // And see if we can chain it to a ternary or binary op.

    using binary_pattern = Seq<binary_op, NodeExpressionUnit>;

    while (auto end = binary_pattern::match(ctx, cursor, b)) {

      while(1) {
        ParseNode* na = nullptr;
        ParseNode* ox = nullptr;
        ParseNode* nb = nullptr;
        ParseNode* oy = nullptr;
        ParseNode* nc = nullptr;

        nc = (end - 1)->top();
        oy = nc ? nc->left_neighbor() : nullptr;
        nb = oy ? oy->left_neighbor() : nullptr;
        ox = nb ? nb->left_neighbor() : nullptr;
        na = ox ? ox->left_neighbor() : nullptr;

        if (!na || !ox || !nb || !oy || !nc) break;

        if (ox->precedence < oy->precedence) {
          //printf("precedence UP\n");
          auto node = new NodeExpressionBinary();
          node->init_span(na->tok_a, nb->tok_b);
        }
        else {
          break;
        }
      }

      cursor = end;
    }

    while(1) {
      ParseNode* nb = nullptr;
      ParseNode* oy = nullptr;
      ParseNode* nc = nullptr;

      nc = (cursor - 1)->top();
      oy = nc ? nc->left_neighbor() : nullptr;
      nb = oy ? oy->left_neighbor() : nullptr;

      if (nb && oy && nc) {
        auto node = new NodeExpressionBinary();
        node->init_span(nb->tok_a, nc->tok_b);
      }
      else {
        break;
      }
    }

    using ternary_pattern = // Not covered by csmith
    Seq<
      // pr68249.c - ternary option can be empty
      // pr49474.c - ternary branches can be comma-lists
      NodeOpBinary<"?">,
      Opt<comma_separated<NodeExpression>>,
      NodeOpBinary<":">,
      Opt<comma_separated<NodeExpression>>
    >;

    if (auto end = ternary_pattern::match(ctx, cursor, b)) {
      auto node = new NodeExpressionTernary();
      node->init_span(a, end - 1);
      cursor = end;
      return cursor;
    }


    return cursor;
  }

  static Token* match(void* ctx, Token* a, Token* b) {
    auto result = match2(ctx, a, b);
    /*
    if (result) {
      int node_count = 0;
      for (auto c = a + 1; c < b; c++) {
        if (c->node_r) dump_tree(c->node_r, 0, 0);
      }
    }
    */
    return result;
  }

};

//------------------------------------------------------------------------------
// 20010911-1.c - Attribute can be empty

struct NodeAttribute : public NodeSpanMaker<NodeAttribute> {

  using pattern = Seq<
    Oneof<
      NodeKeyword<"__attribute__">,
      NodeKeyword<"__attribute">
    >,
    NodePunc<"((">,
    Opt<comma_separated<NodeExpression>>,
    NodePunc<"))">,
    Opt<NodeAttribute>
  >;
};

//------------------------------------------------------------------------------

struct NodeQualifier : public PatternWrapper<NodeQualifier> {
  static Token* match_qualifier(void* ctx, Token* a, Token* b) {
    if (!a || a == b) return nullptr;
    auto result = SST<qualifiers>::match(a->lex->span_a, a->lex->span_b);

    if (result) {
      // FIXME
      auto node = new NodeSpan();
      node->init_span(a, a);
    }

    return result ? a + 1 : nullptr;
  }

  // This is the slowest matcher in the app, why?
  using pattern = Oneof<
    Seq<
      NodeKeyword<"_Alignas">,
      NodePunc<"(">,
      Oneof<
        NodeTypeDecl,
        NodeConstant
      >,
      NodePunc<")">
    >,
    Seq<
      NodeKeyword<"__declspec">,
      NodePunc<"(">,
      NodeIdentifier,
      NodePunc<")">
    >,
    NodeAttribute,
    Ref<match_qualifier>
  >;
};

//------------------------------------------------------------------------------

struct NodeQualifiers : public NodeSpanMaker<NodeQualifiers> {
  using pattern = Some<NodeQualifier>;
};

//------------------------------------------------------------------------------

struct NodePointer : public NodeSpanMaker<NodePointer> {
  using pattern =
  Seq<
    NodeOpPrefix<"*">,
    Any<
      NodeOpPrefix<"*">,
      NodeQualifier
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeParam : public NodeSpanMaker<NodeParam> {
  using pattern = Oneof<
    NodePunc<"...">,
    Seq<
      Opt<NodeQualifiers>,
      NodeSpecifier,
      Opt<NodeQualifiers>,
      Opt<
        NodeDeclarator,
        NodeAbstractDeclarator
      >
    >,
    NodeIdentifier
  >;
};

//------------------------------------------------------------------------------

struct NodeParamList : public NodeSpanMaker<NodeParamList> {
  using pattern = Seq<
    NodePunc<"(">,
    Opt<comma_separated<NodeParam>>,
    NodePunc<")">
  >;
};

//------------------------------------------------------------------------------

struct NodeArraySuffix : public NodeSpanMaker<NodeArraySuffix> {
  using pattern = Oneof<
    Seq<NodePunc<"[">, Opt<NodeQualifiers>,                          Opt<NodeExpression>, NodePunc<"]">>,
    Seq<NodePunc<"[">, NodeKeyword<"static">, Opt<NodeQualifiers>,       NodeExpression,  NodePunc<"]">>,
    Seq<NodePunc<"[">, Opt<NodeQualifiers>,   NodeKeyword<"static">,     NodeExpression,  NodePunc<"]">>,
    Seq<NodePunc<"[">, Opt<NodeQualifiers>,   NodePunc<"*">,                              NodePunc<"]">>
  >;
};

//------------------------------------------------------------------------------

struct NodeTemplateArgs : public NodeSpanMaker<NodeTemplateArgs> {
  using pattern = Delimited<'<', '>',
    Opt<comma_separated<NodeExpression>>
  >;
};

//------------------------------------------------------------------------------

struct NodeAtomicType : public NodeSpanMaker<NodeAtomicType> {
  using pattern = Seq<
    NodeKeyword<"_Atomic">,
    NodePunc<"(">,
    NodeTypeDecl,
    NodePunc<")">
  >;
};

//------------------------------------------------------------------------------

struct NodeSpecifier : public NodeSpanMaker<NodeSpecifier> {
  using pattern = Seq<
    Oneof<
      // FIXME shouldn't these be match_class_name or whatev?
      Seq<NodeKeyword<"class">,  NodeIdentifier>,
      Seq<NodeKeyword<"union">,  NodeIdentifier>,
      Seq<NodeKeyword<"struct">, NodeIdentifier>,
      Seq<NodeKeyword<"enum">,   NodeIdentifier>,
      NodeBuiltinType,
      Ref<&C99Parser::match_typedef_type>,
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
          NodeKeyword<"__typeof__">,
          NodeKeyword<"__typeof">,
          NodeKeyword<"typeof">
        >,
        NodePunc<"(">,
        NodeExpression,
        NodePunc<")">
      >
    >,
    Opt<NodeTemplateArgs>
  >;
};

//------------------------------------------------------------------------------
// (6.7.6) type-name:
//   specifier-qualifier-list abstract-declaratoropt

struct NodeTypeName : public NodeSpanMaker<NodeTypeName> {
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

struct NodeBitSuffix : public NodeSpanMaker<NodeBitSuffix> {
  using pattern = Seq< NodePunc<":">, NodeExpression >;
};

//------------------------------------------------------------------------------

struct NodeAsmSuffix : public NodeSpanMaker<NodeAsmSuffix> {
  using pattern = Seq<
    Oneof<
      NodeKeyword<"asm">,
      NodeKeyword<"__asm">,
      NodeKeyword<"__asm__">
    >,
    NodePunc<"(">,
    Some<NodeString>,
    NodePunc<")">
  >;
};

//------------------------------------------------------------------------------

struct NodeAbstractDeclarator : public NodeSpanMaker<NodeAbstractDeclarator> {
  using pattern =
  Seq<
    Trace<"ptr", Opt<NodePointer>>,
    Opt<Seq<NodePunc<"(">, NodeAbstractDeclarator, NodePunc<")">>>,
    Any<
      NodeAttribute,
      NodeArraySuffix,
      NodeParamList
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeDeclarator : public NodeSpanMaker<NodeDeclarator> {
  using pattern = Seq<
    Opt<NodeAttribute>,
    Opt<NodePointer>,
    Opt<NodeAttribute>,
    Opt<NodeQualifiers>,
    Oneof<
      NodeIdentifier,
      Seq<NodePunc<"(">, NodeDeclarator, NodePunc<")">>
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

struct NodeAccessSpecifier : public NodeSpanMaker<NodeAccessSpecifier> {
  using pattern = Seq<
    Oneof<
      NodeKeyword<"public">,
      NodeKeyword<"private">
    >,
    NodePunc<":">
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

struct NodeFieldList : public NodeSpanMaker<NodeFieldList> {
  using pattern = Seq<
    NodePunc<"{">,
    Any<
      NodePunc<";">,
      NodeField
    >,
    NodePunc<"}">
  >;
};

//------------------------------------------------------------------------------

struct NodeNamespace : public NodeSpanMaker<NodeNamespace> {
  using pattern = Seq<
    NodeKeyword<"namespace">,
    Opt<NodeIdentifier>,
    Opt<NodeFieldList>
  >;
};

//------------------------------------------------------------------------------

struct NodeDeclaratorList : public NodeSpanMaker<NodeDeclaratorList> {
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
        NodePunc<"=">,
        NodeInitializer
      >>
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeStructType  : public NodeSpanMaker<NodeStructType> {
  static Token* match(void* ctx, Token* a, Token* b) {
    auto p = ((C99Parser*)ctx);
    auto end = p->match_struct_type(a, b);
    if (end) {
      auto node = new NodeStructType();
      node->init_span(a, end - 1);
    }
    return end;
  }
};

struct NodeStructDecl : public NodeSpanMaker<NodeStructDecl> {
  using pattern = Seq<
    Opt<NodeQualifiers>,
    NodeKeyword<"struct">,
    Any<NodeAttribute>, // This has to be here, there are a lot of struct __attrib__() foo {};
    Opt<NodeIdentifier>
  >;

  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = NodeSpanMaker::match(ctx, a, b);
    if (end) {
      auto node = a->span;
      if (auto id = node->child<NodeIdentifier>()) {
        //printf("Adding struct type %s\n", id->text().c_str());
        ((C99Parser*)ctx)->add_struct_type(id->text());
      }
    }
    return end;
  }
};

struct NodeStruct : public NodeSpanMaker<NodeStruct> {
  using pattern = Seq<
    NodeStructDecl,
    Opt<NodeFieldList>,
    Any<NodeAttribute>,
    Opt<NodeDeclaratorList>
  >;
};

//------------------------------------------------------------------------------

struct NodeUnionDecl : public NodeSpanMaker<NodeUnionDecl> {
  using pattern = Seq<
    Opt<NodeQualifiers>,
    NodeKeyword<"union">,
    Any<NodeAttribute>,
    Opt<NodeIdentifier>
  >;

  // We specialize match() to dig out typedef'd identifiers
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = NodeSpanMaker::match(ctx, a, b);
    if (end) {
      auto node = a->span;
      if (auto id = node->child<NodeIdentifier>()) {
        //printf("Adding union type %s\n", id->text().c_str());
        ((C99Parser*)ctx)->add_union_type(id->text());
      }
    }
    return end;
  }
};


struct NodeUnion : public NodeSpanMaker<NodeUnion> {
  using pattern = Seq<
    NodeUnionDecl,
    Opt<NodeFieldList>,
    Any<NodeAttribute>,
    Opt<NodeDeclaratorList>
  >;
};

//------------------------------------------------------------------------------

struct NodeClassDecl : public NodeSpanMaker<NodeClassDecl> {
  using pattern = Seq<
    Opt<NodeQualifiers>,
    NodeKeyword<"class">,
    Any<NodeAttribute>,
    Opt<NodeIdentifier>
  >;

  // We specialize match() to dig out typedef'd identifiers
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = NodeSpanMaker::match(ctx, a, b);
    if (end) {
      auto node = a->span;
      if (auto id = node->child<NodeIdentifier>()) {
        //printf("Adding class type %s\n", id->text().c_str());
        ((C99Parser*)ctx)->add_class_type(id->text());
      }
    }
    return end;
  }
};

struct NodeClass : public NodeSpanMaker<NodeClass> {
  using pattern = Seq<
    NodeClassDecl,
    Opt<NodeFieldList>,
    Any<NodeAttribute>,
    Opt<NodeDeclaratorList>
  >;
};

//------------------------------------------------------------------------------

struct NodeTemplateParams : public NodeSpanMaker<NodeTemplateParams> {
  using pattern = Delimited<'<', '>',
    comma_separated<NodeDeclaration>
  >;
};

struct NodeTemplate : public NodeSpanMaker<NodeTemplate> {
  using pattern = Seq<
    NodeKeyword<"template">,
    NodeTemplateParams,
    NodeClass
  >;
};

//------------------------------------------------------------------------------
// FIXME should probably have a few diffeerent versions instead of all the opts

struct NodeEnumName : public NodeSpanMaker<NodeEnumName> {
  using pattern = Seq<
    Opt<NodeQualifiers>,
    NodeKeyword<"enum">,
    Opt<Keyword<"class">>,
    Opt<NodeIdentifier>,
    Opt<Seq<NodePunc<":">, NodeTypeDecl>>
  >;

  // We specialize match() to dig out typedef'd identifiers
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = NodeSpanMaker::match(ctx, a, b);
    if (end) {
      auto node = a->span;
      if (auto id = node->child<NodeIdentifier>()) {
        //printf("Adding enum type %s\n", id->text().c_str());
        ((C99Parser*)ctx)->add_enum_type(id->text());
      }
    }
    return end;
  }
};

struct NodeEnumerator : public NodeSpanMaker<NodeEnumerator> {
  using pattern = Seq<
    NodeIdentifier,
    Opt<Seq<NodePunc<"=">, NodeExpression>>
  >;
};

struct NodeEnumerators : public NodeSpanMaker<NodeEnumerators> {
  using pattern = Seq<
    NodePunc<"{">,
    comma_separated<NodeEnumerator>,
    Opt<NodePunc<",">>,
    NodePunc<"}">
  >;
};

struct NodeEnum : public NodeSpanMaker<NodeEnum> {
  using pattern = Seq<
    NodeEnumName,
    Opt<NodeEnumerators>,
    Opt<NodeDeclaratorList>
  >;
};

//------------------------------------------------------------------------------

struct NodeTypeDecl : public NodeSpanMaker<NodeTypeDecl> {
  using pattern = Seq<
    Opt<NodeQualifiers>,
    NodeSpecifier,
    Opt<NodeAbstractDeclarator>
  >;
};

//------------------------------------------------------------------------------

struct NodeDesignation : public NodeSpanMaker<NodeDesignation> {
  using pattern = Trace<"desig",
  Some<
    Seq<NodePunc<"[">, NodeConstant,   NodePunc<"]">>,
    Seq<NodePunc<"[">, NodeIdentifier, NodePunc<"]">>,
    Seq<NodePunc<".">, NodeIdentifier>
  >>;
};

struct NodeInitializerList : public NodeSpanMaker<NodeInitializerList> {
  using pattern = Seq<
    NodePunc<"{">,
    opt_comma_separated<
      Seq<
        Opt<
          Seq<NodeDesignation, NodePunc<"=">>,
          Seq<NodeIdentifier,  NodePunc<":">> // This isn't in the C grammar but compndlit-1.c uses it?
        >,
        NodeInitializer
      >
    >,
    //Opt<Atom<','>>,
    NodePunc<"}">
  >;
};

struct NodeInitializer : public NodeSpanMaker<NodeInitializer> {
  using pattern = Oneof<
    NodeInitializerList,
    NodeExpression
  >;
};

//------------------------------------------------------------------------------

struct NodeFunctionIdentifier : public NodeSpanMaker<NodeFunctionIdentifier> {
  using pattern = Seq<
    Opt<NodePointer>,
    Opt<NodeAttribute>,
    Oneof<
      NodeIdentifier,
      Seq<NodePunc<"(">, NodeFunctionIdentifier, NodePunc<")">>
    >
  >;
};


struct NodeFunction : public NodeSpanMaker<NodeFunction> {
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
      Seq<NodeDeclaration, NodePunc<";">>
    >>,
    Oneof<
      NodePunc<";">,
      NodeStatementCompound
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeConstructor : public NodeSpanMaker<NodeConstructor> {
  using match_class_type  = Ref<&C99Parser::match_class_type>;

  using pattern = Seq<
    Oneof<
      //match_class_type,
      NodeStructType
    >,
    NodeParamList,
    Oneof<
      NodePunc<";">,
      NodeStatementCompound
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeDeclaration : public NodeSpanMaker<NodeDeclaration> {
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

struct NodeStatementCompound : public NodeSpanMaker<NodeStatementCompound> {
  using pattern = Seq<
    NodePunc<"{">,
    Any<NodeStatement>,
    NodePunc<"}">
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementDeclaration : public NodeSpanMaker<NodeStatementDeclaration> {
  using pattern = Seq<
    NodeDeclaration,
    NodePunc<";">
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementExpression : public NodeSpanMaker<NodeStatementExpression> {
  using pattern = comma_separated<NodeExpression>;
};

//------------------------------------------------------------------------------

struct NodeStatementFor : public NodeSpanMaker<NodeStatementFor> {
  using pattern = Seq<
    NodeKeyword<"for">,
    NodePunc<"(">,
    Oneof<
      Seq<comma_separated<NodeExpression>, NodePunc<";">>,
      Seq<comma_separated<NodeDeclaration>, NodePunc<";">>,
      NodePunc<";">
    >,
    Opt<comma_separated<NodeExpression>>,
    NodePunc<";">,
    Opt<comma_separated<NodeExpression>>,
    NodePunc<")">,
    NodeStatement
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementElse : public NodeSpanMaker<NodeStatementElse> {
  using pattern =
  Seq<
    NodeKeyword<"else">,
    NodeStatement
  >;
};

struct NodeStatementIf : public NodeSpanMaker<NodeStatementIf> {
  using pattern = Seq<
    NodeKeyword<"if">,
    Seq<
      NodePunc<"(">,
      comma_separated<NodeExpression>,
      NodePunc<")">
    >,
    NodeStatement,
    Opt<NodeStatementElse>
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementReturn : public NodeSpanMaker<NodeStatementReturn> {
  using pattern = Seq<
    NodeKeyword<"return">,
    NodeExpression,
    NodePunc<";">
  >;
};


//------------------------------------------------------------------------------

struct NodeStatementCase : public NodeSpanMaker<NodeStatementCase> {
  using pattern = Seq<
    NodeKeyword<"case">,
    NodeExpression,
    Opt<Seq<
      // case 1...2: - this is supported by GCC?
      NodePunc<"...">,
      NodeExpression
    >>,
    NodePunc<":">,
    Any<Seq<
      Not<Keyword<"case">>,
      Not<Keyword<"default">>,
      NodeStatement
    >>
  >;
};

struct NodeStatementDefault : public NodeSpanMaker<NodeStatementDefault> {
  using pattern = Seq<
    NodeKeyword<"default">,
    NodePunc<":">,
    Any<Seq<
      Not<Keyword<"case">>,
      Not<Keyword<"default">>,
      NodeStatement
    >>
  >;
};

struct NodeStatementSwitch : public NodeSpanMaker<NodeStatementSwitch> {
  using pattern = Seq<
    NodeKeyword<"switch">,
    NodeExpression,
    NodePunc<"{">,
    Any<
      NodeStatementCase,
      NodeStatementDefault
    >,
    NodePunc<"}">
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementWhile : public NodeSpanMaker<NodeStatementWhile> {
  using pattern = Seq<
    NodeKeyword<"while">,
    NodePunc<"(">,
    comma_separated<NodeExpression>,
    NodePunc<")">,
    NodeStatement
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementDoWhile : public NodeSpanMaker<NodeStatementDoWhile> {
  using pattern = Seq<
    NodeKeyword<"do">,
    NodeStatement,
    NodeKeyword<"while">,
    NodePunc<"(">,
    NodeExpression,
    NodePunc<")">,
    NodePunc<";">
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementLabel: public NodeSpanMaker<NodeStatementLabel> {
  using pattern = Seq<
    NodeIdentifier,
    NodePunc<":">,
    Opt<NodePunc<";">>
  >;
};

//------------------------------------------------------------------------------

struct NodeAsmRef : public NodeSpanMaker<NodeAsmRef> {
  using pattern = Seq<
    NodeString,
    Opt<Seq<
      NodePunc<"(">,
      NodeExpression,
      NodePunc<")">
    >>
  >;
};

struct NodeAsmRefs : public NodeSpanMaker<NodeAsmRefs> {
  using pattern = comma_separated<NodeAsmRef>;
};

//------------------------------------------------------------------------------

struct NodeAsmQualifiers : public NodeSpanMaker<NodeAsmQualifiers> {
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

struct NodeStatementAsm : public NodeSpanMaker<NodeStatementAsm> {
  using pattern = Seq<
    Oneof<
      NodeKeyword<"asm">,
      NodeKeyword<"__asm">,
      NodeKeyword<"__asm__">
    >,
    //NodeQualifiers,
    Opt<NodeAsmQualifiers>,
    NodePunc<"(">,
    NodeString, // assembly code
    SeqOpt<
      // output operands
      Seq<NodePunc<":">, Opt<NodeAsmRefs>>,
      // input operands
      Seq<NodePunc<":">, Opt<NodeAsmRefs>>,
      // clobbers
      Seq<NodePunc<":">, Opt<NodeAsmRefs>>,
      // GotoLabels
      Seq<NodePunc<":">, Opt<comma_separated<NodeIdentifier>>>
    >,
    NodePunc<")">,
    NodePunc<";">
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementTypedef : public NodeSpanMaker<NodeStatementTypedef> {
  using pattern = Seq<
    Opt<NodeKeyword<"__extension__">>,
    NodeKeyword<"typedef">,
    Oneof<
      NodeStruct,
      NodeUnion,
      NodeClass,
      NodeEnum,
      NodeDeclaration
    >,
    NodePunc<";">
  >;

  static void extract_declarator(void* ctx, const NodeDeclarator* decl) {
    if (auto id = decl->child<NodeIdentifier>()) {
      ((C99Parser*)ctx)->add_typedef_type(id->text());
    }

    for (auto child : decl) {
      if (auto decl = child->as_a<NodeDeclarator>()) {
        extract_declarator(ctx, decl);
      }
    }
  }

  static void extract_declarator_list(void* ctx, const NodeDeclaratorList* decls) {
    if (!decls) return;
    for (auto child : decls) {
      if (auto decl = child->as_a<NodeDeclarator>()) {
        extract_declarator(ctx, decl);
      }
    }
  }

  static void extract_type(void* ctx, Token* a, Token* b) {
    auto node = a->top();

    //node->dump_tree();

    if (auto type = node->child<NodeStruct>()) {
      extract_declarator_list(ctx, type->child<NodeDeclaratorList>());
      return;
    }

    if (auto type = node->child<NodeUnion>()) {
      extract_declarator_list(ctx, type->child<NodeDeclaratorList>());
      return;
    }

    if (auto type = node->child<NodeClass>()) {
      extract_declarator_list(ctx, type->child<NodeDeclaratorList>());
      return;
    }

    if (auto type = node->child<NodeEnum>()) {
      extract_declarator_list(ctx, type->child<NodeDeclaratorList>());
      return;
    }

    if (auto type = node->child<NodeDeclaration>()) {
      extract_declarator_list(ctx, type->child<NodeDeclaratorList>());
      return;
    }

    assert(false);
  }

  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = NodeSpanMaker::match(ctx, a, b);
    if (end) extract_type(ctx, a, b);
    return end;
  }
};

//------------------------------------------------------------------------------

// pr21356.c - Spec says goto should be an identifier, GCC allows expressions
struct NodeStatementGoto : public NodeSpanMaker<NodeStatementGoto> {
  using pattern = Seq<
    NodeKeyword<"goto">,
    NodeExpression,
    NodePunc<";">
  >;
};

//------------------------------------------------------------------------------

struct NodeStatement : public PatternWrapper<NodeStatement> {
  using pattern = Oneof<
    // All of these have keywords or something first
    Seq<NodeClass,  NodePunc<";">>,
    Seq<NodeStruct, NodePunc<";">>,
    Seq<NodeUnion,  NodePunc<";">>,
    Seq<NodeEnum,   NodePunc<";">>,

    NodeStatementTypedef,
    NodeStatementFor,
    NodeStatementIf,
    NodeStatementReturn,
    NodeStatementSwitch,
    NodeStatementDoWhile,
    NodeStatementWhile,
    NodeStatementGoto,
    NodeStatementAsm,
    NodeStatementCompound,

    // These don't - but they might confuse a keyword with an identifier...
    NodeStatementLabel,
    NodeFunction,
    NodeStatementDeclaration,
    Seq<NodeStatementExpression,  NodePunc<";">>,

    // Extra semicolons
    NodePunc<";">
  >;
};

//------------------------------------------------------------------------------
