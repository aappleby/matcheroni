// CONTEXT
// TOP TYPEID

#pragma once

#include "ParseNode.h"
#include <string.h>
#include <array>

struct NodeAbstractDeclarator;
struct NodeAttribute;
struct NodeClass;
struct NodeConstructor;
struct NodeDeclaration;
struct NodeDeclarator;
struct NodeEnum;
struct NodeExpression;
struct NodeExpressionSoup;
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
struct NodeTemplate;
struct NodeTypeDecl;
struct NodeTypeName;
struct NodeUnion;

struct NodeClassType   : public ParseNode {};
struct NodeUnionType   : public ParseNode {};
struct NodeEnumType    : public ParseNode {};
struct NodeTypedefType : public ParseNode {};

//------------------------------------------------------------------------------
// Sorted string table matcher thing.

template<const auto& table>
struct SST;

template<typename T, auto N, const std::array<T, N>& table>
struct SST<table> {

  constexpr inline static int top_bit(int x) {
    for (int b = 31; b >= 0; b--) {
      if (x & (1 << b)) return (1 << b);
    }
    return 0;
  }

  static const char* match(const char* a, const char* b) {
    int bit = top_bit(N);
    int index = 0;

    // I'm not actually sure if 8 is the best tradeoff but it seems OK
    if (N > 8) {
      // Binary search for large tables
      while(1) {
        auto new_index = index | bit;
        if (new_index < N) {
          auto lit = table[new_index];
          auto c = cmp_span_lit(a, b, lit);
          if (c == 0) return lit;
          if (c > 0) index = new_index;
        }
        if (bit == 0) return nullptr;
        bit >>= 1;
      }
    }
    else {
      // Linear scan for small tables
      for (auto lit : table) {
        if (cmp_span_lit(a, b, lit) == 0) return a + 1;
      }
    }

    return nullptr;
  }
};

//------------------------------------------------------------------------------
// MUST BE SORTED CASE-SENSITIVE

constexpr std::array builtin_type_base = {
  "FILE", // used in fprintf.c torture test
  "_Bool",
  "_Complex", // yes this is both a prefix and a type :P
  "__INT16_TYPE__",
  "__INT32_TYPE__",
  "__INT64_TYPE__",
  "__INT8_TYPE__",
  "__INTMAX_TYPE__",
  "__INTPTR_TYPE__",
  "__INT_FAST16_TYPE__",
  "__INT_FAST32_TYPE__",
  "__INT_FAST64_TYPE__",
  "__INT_FAST8_TYPE__",
  "__INT_LEAST16_TYPE__",
  "__INT_LEAST32_TYPE__",
  "__INT_LEAST64_TYPE__",
  "__INT_LEAST8_TYPE__",
  "__PTRDIFF_TYPE__",
  "__SIG_ATOMIC_TYPE__",
  "__SIZE_TYPE__",
  "__UINT16_TYPE__",
  "__UINT32_TYPE__",
  "__UINT64_TYPE__",
  "__UINT8_TYPE__",
  "__UINTMAX_TYPE__",
  "__UINTPTR_TYPE__",
  "__UINT_FAST16_TYPE__",
  "__UINT_FAST32_TYPE__",
  "__UINT_FAST64_TYPE__",
  "__UINT_FAST8_TYPE__",
  "__UINT_LEAST16_TYPE__",
  "__UINT_LEAST32_TYPE__",
  "__UINT_LEAST64_TYPE__",
  "__UINT_LEAST8_TYPE__",
  "__WCHAR_TYPE__",
  "__WINT_TYPE__",
  "__builtin_va_list",
  "__imag__",
  "__int128",
  "__real__",
  "bool",
  "char",
  "double",
  "float",
  "int",
  "int16_t",
  "int32_t",
  "int64_t",
  "int8_t",
  "long",
  "short",
  "signed",
  "size_t", // used in fputs-lib.c torture test
  "uint16_t",
  "uint32_t",
  "uint64_t",
  "uint8_t",
  "unsigned",
  "va_list", // technically part of the c library, but it shows up in stdarg test files
  "void",
  "wchar_t",
};

// MUST BE SORTED CASE-SENSITIVE
constexpr std::array builtin_type_prefix = {
  "_Complex",
  "__complex__",
  "__imag__",
  "__real__",
  "__signed__",
  "__unsigned__",
  "long",
  "short",
  "signed",
  "unsigned",
};

// MUST BE SORTED CASE-SENSITIVE
constexpr std::array builtin_type_suffix = {
  // Why, GCC, why?
  "_Complex",
  "__complex__",
};

// MUST BE SORTED CASE-SENSITIVE
constexpr std::array qualifiers = {
  "_Noreturn",
  "_Thread_local",
  "__const",
  "__extension__",
  "__inline",
  "__inline__",
  "__restrict",
  "__restrict__",
  "__stdcall",
  "__thread",
  "__volatile",
  "__volatile__",
  "auto",
  "const",
  "consteval",
  "constexpr",
  "constinit",
  "explicit",
  "extern",
  "inline",
  "mutable",
  "register",
  "restrict",
  "static",
  "thread_local",
  "virtual",
  "volatile",
};

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

template<typename P>
using comma_separated = Seq<P, Any<Seq<Atom<','>, P>>, Opt<Atom<','>> >;

template<typename P>
using opt_comma_separated = Opt<comma_separated<P>>;

//------------------------------------------------------------------------------

template<StringParam lit>
struct Operator {
  static Token* match(void* ctx, Token* a, Token* b) {
    if (!a || a == b) return nullptr;
    if (a + lit.len > b) return nullptr;

    for (auto i = 0; i < lit.len; i++) {
      if (!a[i].is_punct(lit.value[i])) return nullptr;
    }

    return a + lit.len;
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

  // used by "add_*_type"
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
// Our builtin types are any sequence of prefixes followed by a builtin type

struct NodeBuiltinType : public NodeMaker<NodeBuiltinType> {
  using match_base   = Ref<&C99Parser::match_builtin_type_base>;
  using match_prefix = Ref<&C99Parser::match_builtin_type_prefix>;
  using match_suffix = Ref<&C99Parser::match_builtin_type_suffix>;

  using pattern = Seq<
    Any<Seq<match_prefix, And<match_base>>>,
    match_base,
    Opt<match_suffix>
  >;
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
// This is a weird ({...}) thing that GCC supports

struct NodeGccCompoundExpression : public NodeMaker<NodeGccCompoundExpression> {
  using pattern = Seq<
    Atom<'('>,
    NodeStatementCompound,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------
// This captures all expresion forms, but does _not_ bother to put them into a
// tree or do operator precedence or anything.

// FIXME - replace with some other parser

struct NodeExpressionTernary : public ParseNode {};
struct NodeExpressionBinary  : public ParseNode {};
struct NodeExpressionPrefix  : public ParseNode {};
struct NodeExpressionPostfix : public ParseNode {};

struct NodeExpressionSoup {
  constexpr inline static const char* op1 = "-!*&+~";

  static Token* match_operators(void* ctx, Token* a, Token* b) {
    if (!a || a == b) return nullptr;

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

  //----------------------------------------

  using prefix_pattern =
  Seq<
    Oneof<
      NodeOperator<"++">,
      NodeOperator<"--">,
      NodeOperator<"+">,
      NodeOperator<"-">,
      NodeOperator<"!">,
      NodeOperator<"~">,
      NodeOperator<"*">,
      NodeOperator<"&">
    >,
    NodeExpressionSoup
  >;

  //----------------------------------------

  using postfix_pattern =
  Oneof<
    NodeOperator<"++">,
    NodeOperator<"--">
  >;

  //----------------------------------------

  using ternary_pattern = // Not covered by csmith
  Seq<
    // pr68249.c - ternary option can be empty
    // pr49474.c - ternary branches can be comma-lists
    NodeOperator<"?">,
    Opt<comma_separated<NodeExpressionSoup>>,
    NodeOperator<":">,
    Opt<comma_separated<NodeExpressionSoup>>
  >;

  //----------------------------------------

  using binary_pattern = // Not covered by torture or csmith
  Seq<
    Oneof<
      NodeOperator<"...">,
      NodeOperator<"<<=">,
      NodeOperator<">>=">,
      NodeOperator<"->*">,
      NodeOperator<"<=>">,
      NodeOperator<".*">,
      NodeOperator<"::">,
      NodeOperator<"-=">,
      NodeOperator<"->">,
      NodeOperator<"!=">,
      NodeOperator<"*=">,
      NodeOperator<"/=">,
      NodeOperator<"&=">,
      NodeOperator<"%=">,
      NodeOperator<"^=">,
      NodeOperator<"+=">,
      NodeOperator<"<<">,
      NodeOperator<"<=">,
      NodeOperator<"==">,
      NodeOperator<">=">,
      NodeOperator<">>">,
      NodeOperator<"|=">,
      NodeOperator<"||">,
      NodeOperator<"&&">,
      NodeOperator<".">,
      NodeOperator<"/">,
      NodeOperator<"&">,
      NodeOperator<"%">,
      NodeOperator<"^">,
      NodeOperator<"<">,
      NodeOperator<"=">,
      NodeOperator<">">,
      NodeOperator<"|">
    >,
    NodeExpressionSoup
  >;

  //----------------------------------------

  static Token* match(void* ctx, Token* a, Token* b) {
    if (auto end = prefix_pattern::match(ctx, a, b)) {
      auto node = new NodeExpressionPrefix();
      node->init(a, end);
      return end;
    }

    auto end = pattern::match(ctx, a, b);

    if (auto end2 = postfix_pattern::match(ctx, end, b)) {
      auto node = new NodeExpressionPostfix();
      node->init(a, end2);
      return end2;
    }

    if (auto end2 = ternary_pattern::match(ctx, end, b)) {
      auto node = new NodeExpressionTernary();
      node->init(a, end2);
      return end2;
    }

    if (auto end2 = binary_pattern::match(ctx, end, b)) {
      auto node = new NodeExpressionBinary();
      node->init(a, end2);
      return end2;
    }

    return end;
  }
};

struct NodeExpression : public NodeMaker<NodeExpression> {
  using pattern = NodeExpressionSoup;
};

//------------------------------------------------------------------------------
// 20010911-1.c - Attribute can be empty

struct NodeAttribute : public NodeMaker<NodeAttribute> {

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
  static Token* match_qualifier(void* ctx, Token* a, Token* b) {
    if (!a || a == b) return nullptr;
    auto result = SST<qualifiers>::match(a->lex->span_a, a->lex->span_b);
    return result ? a + 1 : nullptr;
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

struct NodeClassName;
struct NodeStructType;

struct NodeSpecifier : public NodeMaker<NodeSpecifier> {
  using pattern = Seq<
    Oneof<
      // FIXME shouldn't these be match_class_name or whatev?
      Seq<Keyword<"class">,  NodeIdentifier>,
      Seq<Keyword<"union">,  NodeIdentifier>,
      Seq<Keyword<"struct">, NodeIdentifier>,
      Seq<Keyword<"enum">,   NodeIdentifier>,
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

struct NodeStructType  : public NodeMaker<NodeStructType> {

  /*
  static Token* match_type(void* ctx, Token* a, Token* b) {
    auto p = ((C99Parser*)ctx);
    return p->match_struct_type(a, b);
  }

  using pattern = Ref<&C99Parser::match_struct_type>;
  */

  /*
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = Ref<&C99Parser::match_struct_type>::match(ctx, a, b);
    if (end) {
      NodeStructType* node = new NodeStructType();
      node->init(a, end);
    }
    return end;
  }
  */

  static Token* match(void* ctx, Token* a, Token* b) {
    auto p = ((C99Parser*)ctx);
    auto end = p->match_struct_type(a, b);
    if (end) {
      NodeStructType* node = new NodeStructType();
      node->init(a, end);
    }
    return end;
  }
};

struct NodeStructName : public NodeMaker<NodeStructName> {
  using pattern = Seq<
    Opt<NodeQualifiers>,
    Keyword<"struct">,
    Any<NodeAttribute>, // This has to be here, there are a lot of struct __attrib__() foo {};
    Opt<NodeIdentifier>
  >;

  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = NodeMaker::match(ctx, a, b);
    if (end) {
      auto node = a->top;
      if (auto id = node->child<NodeIdentifier>()) {
        //printf("Adding struct type %s\n", id->text().c_str());
        ((C99Parser*)ctx)->add_struct_type(id->text());
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
        ((C99Parser*)ctx)->add_union_type(id->text());
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
        ((C99Parser*)ctx)->add_class_type(id->text());
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
        ((C99Parser*)ctx)->add_enum_type(id->text());
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
  using match_class_type  = Ref<&C99Parser::match_class_type>;

  using pattern = Seq<
    Oneof<
      //match_class_type,
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

  static void extract_declarator(void* ctx, const NodeDeclarator* decl) {
    if (auto id = decl->child<NodeIdentifier>()) {
      ((C99Parser*)ctx)->add_typedef_type(id->text());
    }

    for (auto child : decl) {
      if (auto decl = child->as<NodeDeclarator>()) {
        extract_declarator(ctx, decl);
      }
    }
  }

  static void extract_declarator_list(void* ctx, const NodeDeclaratorList* decls) {
    if (!decls) return;
    for (auto child : decls) {
      if (auto decl = child->as<NodeDeclarator>()) {
        extract_declarator(ctx, decl);
      }
    }
  }

  static void extract_type(void* ctx, Token* a, Token* b) {
    auto node = a->top;

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
    auto end = NodeMaker::match(ctx, a, b);
    if (end) extract_type(ctx, a, b);
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
