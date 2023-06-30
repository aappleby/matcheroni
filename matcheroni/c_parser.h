#pragma once

#include "ParseNode.h"
#include "c_constants.h"
#include <algorithm>

void dump_tree(const ParseNode* n, int max_depth, int indentation);

struct BaseClassType;
struct BaseEnumType;
struct BaseStructType;
struct BaseTypedefType;
struct BaseUnionType;

struct SpanAbstractDeclarator;
struct SpanClass;
struct SpanConstructor;
struct SpanDeclaration;
struct SpanDeclarator;
struct SpanEnum;
struct SpanExpression;
struct SpanFunction;
struct SpanInitializer;
struct SpanInitializerList;
struct SpanSpecifier;
struct SpanStatement;
struct SpanStatementCompound;
struct SpanTypedef;
struct SpanStruct;
struct SpanTemplate;
struct SpanTypeDecl;
struct SpanTypeName;
struct SpanUnion;

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

  Token* put_type(std::vector<std::string>& types, Token* a, Token* b) {
    if (!a || a == b) return nullptr;
    if (a->lex->type == LEX_IDENTIFIER) {
      auto t = a->lex->text();
      if (std::find(types.begin(), types.end(), t) == types.end()) {
        types.push_back(t);
      }
      return a + 1;
    }
    else {
      return nullptr;
    }
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

  Token* put_class_type    (Token* a, Token* b) { return put_type(class_types,   a, b); }
  Token* put_struct_type   (Token* a, Token* b) { return put_type(struct_types,  a, b); }
  Token* put_union_type    (Token* a, Token* b) { return put_type(union_types,   a, b); }
  Token* put_enum_type     (Token* a, Token* b) { return put_type(enum_types,    a, b); }
  Token* put_typedef_type  (Token* a, Token* b) { return put_type(typedef_types, a, b); }

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

struct BaseBuiltinType : public BaseMaker<BaseBuiltinType> {
  using match_prefix = Ref<&C99Parser::match_builtin_type_prefix>;
  using match_base   = Ref<&C99Parser::match_builtin_type_base>;
  using match_suffix = Ref<&C99Parser::match_builtin_type_suffix>;

  using pattern = Seq<
    //And<Oneof<Atom<LEX_KEYWORD>, Atom<LEX_IDENTIFIER>>>,
    Any<Seq<match_prefix, And<match_base>>>,
    match_base,
    Opt<match_suffix>
  >;
};

//------------------------------------------------------------------------------

struct BaseTypedefType : public BaseMaker<BaseTypedefType> {
  using pattern = Ref<&C99Parser::match_typedef_type>;
};

//------------------------------------------------------------------------------

struct BaseOpPrefix : public NodeBase {};

template<StringParam lit>
struct MatchOpPrefix {
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = match_punct(ctx, a, b, lit.str_val, lit.str_len);
    if (end) {
      auto node = new BaseOpPrefix();
      node->precedence = prefix_precedence(lit.str_val);
      node->assoc      = prefix_assoc(lit.str_val);
      node->init_base(a, end - 1);
    }
    return end;
  }
};

//------------------------------------------------------------------------------

struct BaseOpBinary : public NodeBase {};

template<StringParam lit>
struct MatchOpBinary {
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = match_punct(ctx, a, b, lit.str_val, lit.str_len);
    if (end) {
      auto node = new BaseOpBinary();
      node->precedence = binary_precedence(lit.str_val);
      node->assoc      = binary_assoc(lit.str_val);
      node->init_base(a, end - 1);
    }
    return end;
  }
};

//------------------------------------------------------------------------------

struct BaseOpSuffix : public NodeBase {};

template<StringParam lit>
struct MatchOpSuffix {
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = match_punct(ctx, a, b, lit.str_val, lit.str_len);
    if (end) {
      auto node = new BaseOpSuffix();
      node->precedence = suffix_precedence(lit.str_val);
      node->assoc      = suffix_assoc(lit.str_val);
      node->init_base(a, end - 1);
    }
    return end;
  }
};

//------------------------------------------------------------------------------

struct BaseQualifier : public BaseMaker<BaseQualifier> {
  static Token* match(void* ctx, Token* a, Token* b) {
    if (!a || a == b) return nullptr;
    auto result = SST<qualifiers>::match(a->lex->span_a, a->lex->span_b);
    if (result) {
      auto node = new BaseQualifier();
      node->init_base(a, a);
      return a + 1;
    }
    else {
      return nullptr;
    }
  }
};

//------------------------------------------------------------------------------

struct BaseAsmSuffix : public BaseMaker<BaseAsmSuffix> {
  using pattern = Seq<
    Oneof<
      FlatKeyword<"asm">,
      FlatKeyword<"__asm">,
      FlatKeyword<"__asm__">
    >,
    FlatAtom<'('>,
    Some<BaseAtom<LEX_STRING>>,
    FlatAtom<')'>
  >;
};

//------------------------------------------------------------------------------

struct BaseAccessSpecifier : public BaseMaker<BaseAccessSpecifier> {
  using pattern = Seq<
    Oneof<
      BaseKeyword<"public">,
      BaseKeyword<"private">
    >,
    FlatAtom<':'>
  >;
};






























































//------------------------------------------------------------------------------
// (6.5.4) cast-expression:
//   unary-expression
//   ( type-name ) cast-expression

struct SpanExpressionCast : public SpanMaker<SpanExpressionCast> {
  using pattern = Seq<
    FlatAtom<'('>,
    SpanTypeName,
    FlatAtom<')'>
  >;
};

//------------------------------------------------------------------------------

struct SpanExpressionParen : public SpanMaker<SpanExpressionParen> {
  using pattern =
  DelimitedList<
    FlatAtom<'('>,
    SpanExpression,
    FlatAtom<','>,
    FlatAtom<')'>
  >;
};

//------------------------------------------------------------------------------

struct SpanExpressionBraces : public SpanMaker<SpanExpressionBraces> {
  using pattern =
  DelimitedList<
    FlatAtom<'{'>,
    SpanExpression,
    FlatAtom<','>,
    FlatAtom<'}'>
  >;
};

//------------------------------------------------------------------------------

struct SpanExpressionSubscript : public SpanMaker<SpanExpressionSubscript> {
  using pattern =
  DelimitedList<
    FlatAtom<'['>,
    SpanExpression,
    FlatAtom<','>,
    FlatAtom<']'>
  >;
};

//------------------------------------------------------------------------------
// This is a weird ({...}) thing that GCC supports

struct SpanExpressionGccCompound : public SpanMaker<SpanExpressionGccCompound> {
  using pattern = Seq<
    Opt<FlatKeyword<"__extension__">>,
    FlatAtom<'('>,
    SpanStatementCompound,
    FlatAtom<')'>
  >;
};

//------------------------------------------------------------------------------

struct SpanExpressionTernary : public NodeSpan {};
struct SpanExpressionBinary  : public NodeSpan {};

struct SpanExpressionSizeof  : public SpanMaker<SpanExpressionSizeof> {
  using pattern = Seq<
    FlatKeyword<"sizeof">,
    Oneof<
      SpanExpressionCast,
      SpanExpressionParen,
      SpanExpression
    >
  >;
};

struct SpanExpressionAlignof  : public SpanMaker<SpanExpressionAlignof> {
  using pattern = Seq<
    FlatKeyword<"__alignof__">,
    Oneof<
      SpanExpressionCast,
      SpanExpressionParen
    >
  >;
};

struct SpanExpressionOffsetof  : public SpanMaker<SpanExpressionOffsetof> {
  using pattern = Seq<
    Oneof<
      FlatKeyword<"offsetof">,
      FlatKeyword<"__builtin_offsetof">
    >,
    FlatAtom<'('>,
    SpanTypeName,
    FlatAtom<','>,
    SpanExpression,
    FlatAtom<')'>
  >;
};

//----------------------------------------

using SpanPrefixOp =
Oneof<
  SpanExpressionCast,
  BaseKeyword<"__extension__">,
  BaseKeyword<"__real">,
  BaseKeyword<"__real__">,
  BaseKeyword<"__imag">,
  BaseKeyword<"__imag__">,
  MatchOpPrefix<"++">,
  MatchOpPrefix<"--">,
  MatchOpPrefix<"+">,
  MatchOpPrefix<"-">,
  MatchOpPrefix<"!">,
  MatchOpPrefix<"~">,
  MatchOpPrefix<"*">,
  MatchOpPrefix<"&">
>;

//----------------------------------------

using SpanCore = Oneof<
  SpanExpressionSizeof,
  SpanExpressionAlignof,
  SpanExpressionOffsetof,
  SpanExpressionGccCompound,
  SpanExpressionParen,
  SpanInitializerList,
  SpanExpressionBraces,
  BaseIdentifier,
  BaseConstant
>;

//----------------------------------------

using SpanSuffixOp =
Oneof<
  SpanInitializerList,   // must be before SpanExpressionBraces
  SpanExpressionBraces,
  SpanExpressionParen,
  SpanExpressionSubscript,
  MatchOpSuffix<"++">,
  MatchOpSuffix<"--">
>;

//----------------------------------------

struct SpanExpressionUnit : public SpanMaker<SpanExpressionUnit> {
  using pattern = Seq<
    Any<SpanPrefixOp>,
    SpanCore,
    Any<SpanSuffixOp>
  >;
};


struct SpanExpression : public NodeSpan {

  static Token* match_binary_op(void* ctx, Token* a, Token* b) {
    if (!a || a == b) return nullptr;

    if (a->lex->type != LEX_PUNCT) return nullptr;

    switch(a->lex->span_a[0]) {
      case '+': return Oneof< MatchOpBinary<"+=">, MatchOpBinary<"+"> >::match(ctx, a, b);
      case '-': return Oneof< MatchOpBinary<"->*">, MatchOpBinary<"->">, MatchOpBinary<"-=">, MatchOpBinary<"-"> >::match(ctx, a, b);
      case '*': return Oneof< MatchOpBinary<"*=">, MatchOpBinary<"*"> >::match(ctx, a, b);
      case '/': return Oneof< MatchOpBinary<"/=">, MatchOpBinary<"/"> >::match(ctx, a, b);
      case '=': return Oneof< MatchOpBinary<"==">, MatchOpBinary<"="> >::match(ctx, a, b);
      case '<': return Oneof< MatchOpBinary<"<<=">, MatchOpBinary<"<=>">, MatchOpBinary<"<=">, MatchOpBinary<"<<">, MatchOpBinary<"<"> >::match(ctx, a, b);
      case '>': return Oneof< MatchOpBinary<">>=">, MatchOpBinary<">=">, MatchOpBinary<">>">, MatchOpBinary<">"> >::match(ctx, a, b);
      case '!': return MatchOpBinary<"!=">::match(ctx, a, b);
      case '&': return Oneof< MatchOpBinary<"&&">, MatchOpBinary<"&=">, MatchOpBinary<"&"> >::match(ctx, a, b);
      case '|': return Oneof< MatchOpBinary<"||">, MatchOpBinary<"|=">, MatchOpBinary<"|"> >::match(ctx, a, b);
      case '^': return Oneof< MatchOpBinary<"^=">, MatchOpBinary<"^"> >::match(ctx, a, b);
      case '%': return Oneof< MatchOpBinary<"%=">, MatchOpBinary<"%"> >::match(ctx, a, b);
      case '.': return Oneof< MatchOpBinary<".*">, MatchOpBinary<"."> >::match(ctx, a, b);
      case ':': return MatchOpBinary<"::">::match(ctx, a, b);
      default:  return nullptr;
    }
  }

  //----------------------------------------

  /*
  Operators that have the same precedence are bound to their arguments in the
  direction of their associativity. For example, the expression a = b = c is
  parsed as a = (b = c), and not as (a = b) = c because of right-to-left
  associativity of assignment, but a + b - c is parsed (a + b) - c and not
  a + (b - c) because of left-to-right associativity of addition and
  subtraction.
  */

  static Token* match2(void* ctx, Token* a, Token* b) {
    Token* cursor = a;

    using pattern = Seq<
      Any<SpanPrefixOp>,
      SpanCore,
      Any<SpanSuffixOp>
    >;
    cursor = SpanExpressionUnit::match(ctx, a, b);
    if (!cursor) return nullptr;

    // And see if we can chain it to a ternary or binary op.

    //using binary_pattern = Seq<BaseBinaryOp, SpanExpressionUnit>;
    using binary_pattern = Seq<Ref<match_binary_op>, SpanExpressionUnit>;

    while (auto end = binary_pattern::match(ctx, cursor, b)) {

      // Fold up as many nodes based on precedence as we can
      while(1) {
        ParseNode*    na = nullptr;
        BaseOpBinary* ox = nullptr;
        ParseNode*    nb = nullptr;
        BaseOpBinary* oy = nullptr;
        ParseNode*    nc = nullptr;

        nc =          (end - 1)->span->as_a<ParseNode>();
        oy = nc ? nc->left_neighbor()->as_a<BaseOpBinary>()   : nullptr;
        nb = oy ? oy->left_neighbor()->as_a<ParseNode>() : nullptr;
        ox = nb ? nb->left_neighbor()->as_a<BaseOpBinary>()   : nullptr;
        na = ox ? ox->left_neighbor()->as_a<ParseNode>() : nullptr;

        if (!na || !ox || !nb || !oy || !nc) break;

        if (na->tok_b < a) break;

        if (ox->precedence < oy->precedence) {
          // Left-associate because right operator is "lower" precedence.
          // "a * b + c" -> "(a * b) + c"
          auto node = new SpanExpressionBinary();
          node->init_span(na->tok_a, nb->tok_b);
        }
        else if (ox->precedence == oy->precedence) {
          DCHECK(ox->assoc == oy->assoc);

          if (ox->assoc == 1) {
            // Left to right
            // "a + b - c" -> "(a + b) - c"
            auto node = new SpanExpressionBinary();
            node->init_span(na->tok_a, nb->tok_b);
          }
          else if (ox->assoc == -1) {
            // Right to left
            // "a = b = c" -> "a = (b = c)"
            auto node = new SpanExpressionBinary();
            node->init_span(nb->tok_a, nc->tok_b);
          }
          else {
            CHECK(false);
          }
        }
        else {
          break;
        }
      }

      cursor = end;
    }

    // Any binary operators left on the tokens are in increasing-precedence
    // order, but since there are no more operators we can just fold them up
    // right-to-left
    while(1) {
      ParseNode*    nb = nullptr;
      BaseOpBinary* oy = nullptr;
      ParseNode*    nc = nullptr;

      nc =      (cursor - 1)->span->as_a<ParseNode>();
      oy = nc ? nc->left_neighbor()->as_a<BaseOpBinary>()   : nullptr;
      nb = oy ? oy->left_neighbor()->as_a<ParseNode>() : nullptr;

      if (!nb || !oy || !nc) break;
      if (nb->tok_b < a) break;

      auto node = new SpanExpressionBinary();
      node->init_span(nb->tok_a, nc->tok_b);
    }

    using SpanTernaryOp = // Not covered by csmith
    Seq<
      // pr68249.c - ternary option can be empty
      // pr49474.c - ternary branches can be comma-lists
      MatchOpBinary<"?">,
      Opt<comma_separated<SpanExpression>>,
      MatchOpBinary<":">,
      Opt<comma_separated<SpanExpression>>
    >;

    if (auto end = SpanTernaryOp::match(ctx, cursor, b)) {
      auto node = new SpanExpressionTernary();
      node->init_span(a, end - 1);
      cursor = end;
    }

    auto node = new SpanExpression();
    node->init_span(a, cursor - 1);

    return cursor;
  }

  static Token* match(void* ctx, Token* a, Token* b) {
    auto result = match2(ctx, a, b);
    return result;
  }

};

//------------------------------------------------------------------------------
// 20010911-1.c - Attribute can be empty

struct SpanAttribute : public SpanMaker<SpanAttribute> {
  using pattern = Seq<
    Oneof<
      FlatKeyword<"__attribute__">,
      FlatKeyword<"__attribute">
    >,
    DelimitedList<
      Seq<FlatAtom<'('>, FlatAtom<'('>>,
      Oneof<
        SpanExpression,
        Keyword<"const"> // __attribute__((const))
      >,
      FlatAtom<','>,
      Seq<FlatAtom<')'>, FlatAtom<')'>>
    >
  >;
};

//------------------------------------------------------------------------------

struct SpanAlignas : public SpanMaker<SpanAlignas> {
  using pattern = Seq<
    FlatKeyword<"_Alignas">,
    FlatAtom<'('>,
    Oneof<
      SpanTypeDecl,
      BaseConstant
    >,
    FlatAtom<')'>
  >;
};

//------------------------------------------------------------------------------

struct SpanDeclspec : public SpanMaker<SpanDeclspec> {
  using pattern = Seq<
    FlatKeyword<"__declspec">,
    FlatAtom<'('>,
    BaseIdentifier,
    FlatAtom<')'>
  >;
};

//------------------------------------------------------------------------------

struct SpanModifier : public PatternWrapper<SpanModifier> {
  // This is the slowest matcher in the app, why?
  using pattern = Oneof<
    SpanAlignas,
    SpanDeclspec,
    SpanAttribute,
    BaseQualifier
  >;
};

//------------------------------------------------------------------------------

struct SpanTypeDecl : public SpanMaker<SpanTypeDecl> {
  using pattern = Seq<
    Any<SpanModifier>,
    SpanSpecifier,
    Opt<SpanAbstractDeclarator>
  >;
};

//------------------------------------------------------------------------------

struct SpanPointer : public SpanMaker<SpanPointer> {
  using pattern =
  Seq<
    MatchOpPrefix<"*">,
    Any<
      MatchOpPrefix<"*">,
      SpanModifier
    >
  >;
};

//------------------------------------------------------------------------------

struct SpanParam : public SpanMaker<SpanParam> {
  using pattern = Oneof<
    BaseOperator<"...">,
    Seq<
      Any<SpanModifier>,
      SpanSpecifier,
      Any<SpanModifier>,
      Opt<
        SpanDeclarator,
        SpanAbstractDeclarator
      >
    >,
    BaseIdentifier
  >;
};

//------------------------------------------------------------------------------

struct SpanParamList : public SpanMaker<SpanParamList> {
  using pattern =
  DelimitedList<
    FlatAtom<'('>,
    SpanParam,
    FlatAtom<','>,
    FlatAtom<')'>
  >;
};

//------------------------------------------------------------------------------

struct SpanArraySuffix : public SpanMaker<SpanArraySuffix> {
  using pattern = Oneof<
    Seq<FlatAtom<'['>,                         Any<SpanModifier>,                           Opt<SpanExpression>, FlatAtom<']'>>,
    Seq<FlatAtom<'['>, BaseKeyword<"static">,  Any<SpanModifier>,                               SpanExpression,  FlatAtom<']'>>,
    Seq<FlatAtom<'['>,                         Any<SpanModifier>,   BaseKeyword<"static">,      SpanExpression,  FlatAtom<']'>>,
    Seq<FlatAtom<'['>,                         Any<SpanModifier>,   BaseOperator<"*">,                           FlatAtom<']'>>
  >;
};

//------------------------------------------------------------------------------

struct SpanTemplateArgs : public SpanMaker<SpanTemplateArgs> {
  using pattern =
  DelimitedList<
    FlatAtom<'<'>,
    SpanExpression,
    FlatAtom<','>,
    FlatAtom<'>'>
  >;
};

//------------------------------------------------------------------------------

struct SpanAtomicType : public SpanMaker<SpanAtomicType> {
  using pattern = Seq<
    FlatKeyword<"_Atomic">,
    FlatAtom<'('>,
    SpanTypeDecl,
    FlatAtom<')'>
  >;
};

//------------------------------------------------------------------------------

struct SpanSpecifier : public SpanMaker<SpanSpecifier> {
  using pattern = Seq<
    Oneof<
      // These have to be BaseIdentifier because "void foo(struct S);" is valid
      // even without the definition of S.
      Seq<FlatKeyword<"class">,  BaseIdentifier>,
      Seq<FlatKeyword<"union">,  BaseIdentifier>,
      Seq<FlatKeyword<"struct">, BaseIdentifier>,
      Seq<FlatKeyword<"enum">,   BaseIdentifier>,

      /*
      // If this was C++, we would also need to match these directly
      BaseClassType,
      BaseStructType,
      NodeUnionType,
      NodeEnumType,
      */

      BaseBuiltinType,
      BaseTypedefType,
      SpanAtomicType,
      Seq<
        Oneof<
          FlatKeyword<"__typeof__">,
          FlatKeyword<"__typeof">,
          FlatKeyword<"typeof">
        >,
        FlatAtom<'('>,
        SpanExpression,
        FlatAtom<')'>
      >
    >,
    Opt<SpanTemplateArgs>
  >;
};

//------------------------------------------------------------------------------
// (6.7.6) type-name:
//   specifier-qualifier-list abstract-declaratoropt

struct SpanTypeName : public SpanMaker<SpanTypeName> {
  using pattern = Seq<
    Some<
      SpanSpecifier,
      SpanModifier
    >,
    Opt<SpanAbstractDeclarator>
  >;
};

//------------------------------------------------------------------------------
// Spec says the bit size can be any constant expression, but can we use just a
// constant or a paren-expression?

// (6.7.2.1) struct-declarator:
//   declarator
//   declaratoropt : constant-expression

struct SpanBitSuffix : public SpanMaker<SpanBitSuffix> {
  using pattern = Seq< FlatAtom<':'>, SpanExpression >;
};

//------------------------------------------------------------------------------

struct SpanAbstractDeclarator : public SpanMaker<SpanAbstractDeclarator> {
  using pattern =
  Seq<
    Opt<SpanPointer>,
    Opt<Seq<FlatAtom<'('>, SpanAbstractDeclarator, FlatAtom<')'>>>,
    Any<
      SpanAttribute,
      SpanArraySuffix,
      SpanParamList
    >
  >;
};

//------------------------------------------------------------------------------

struct SpanDeclarator : public SpanMaker<SpanDeclarator> {
  using pattern = Seq<
    Any<
      SpanAttribute,
      SpanModifier,
      SpanPointer
    >,
    Oneof<
      BaseIdentifier,
      Seq<FlatAtom<'('>, SpanDeclarator, FlatAtom<')'>>
    >,
    Opt<BaseAsmSuffix>,
    Any<
      SpanBitSuffix,
      SpanAttribute,
      SpanArraySuffix,
      SpanParamList
    >
  >;
};

//------------------------------------------------------------------------------

struct SpanDeclaratorList : public SpanMaker<SpanDeclaratorList> {
  using pattern =
  comma_separated<
    Seq<
      Oneof<
        Seq<SpanDeclarator, Opt<SpanBitSuffix> >,
        SpanBitSuffix
      >,
      Opt<Seq<FlatAtom<'='>, SpanInitializer>>
    >
  >;
};

//------------------------------------------------------------------------------

struct SpanField : public PatternWrapper<SpanField> {
  using pattern = Oneof<
    FlatAtom<';'>,
    BaseAccessSpecifier,
    SpanConstructor,
    SpanFunction,
    SpanStruct,
    SpanUnion,
    SpanTemplate,
    SpanClass,
    SpanEnum,
    SpanDeclaration
  >;
};

struct SpanFieldList : public SpanMaker<SpanFieldList> {
  using pattern =
  DelimitedBlock<
    FlatAtom<'{'>,
    SpanField,
    FlatAtom<'}'>
  >;
};

//------------------------------------------------------------------------------

struct SpanNamespace : public SpanMaker<SpanNamespace> {
  using pattern = Seq<
    FlatKeyword<"namespace">,
    Opt<BaseIdentifier>,
    Opt<SpanFieldList>
  >;
};

//------------------------------------------------------------------------------

struct BaseStructType : public BaseMaker<BaseStructType> {
  using pattern = Ref<&C99Parser::match_struct_type>;
};

struct BaseStructTypeAdder : public BaseIdentifier {
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = BaseIdentifier::match(ctx, a, b);
    if (end) {
      ((C99Parser*)ctx)->add_struct_type(a->lex->text());
    }
    return end;
  }
};

struct SpanStruct : public SpanMaker<SpanStruct> {
  using pattern = Seq<
    Any<SpanModifier>,
    FlatKeyword<"struct">,
    Any<SpanAttribute>,    // This has to be here, there are a lot of struct __attrib__() foo {};
    Opt<BaseStructTypeAdder>,
    Opt<SpanFieldList>,
    Any<SpanAttribute>,
    Opt<SpanDeclaratorList>
  >;
};

//------------------------------------------------------------------------------

struct BaseUnionType : public NodeSpan {
  static Token* match(void* ctx, Token* a, Token* b) {
    auto p = ((C99Parser*)ctx);
    auto end = p->match_union_type(a, b);
    if (end) {
      auto node = new BaseUnionType();
      node->init_span(a, end - 1);
    }
    return end;
  }
};

struct BaseUnionTypeAdder : public BaseIdentifier {
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = BaseIdentifier::match(ctx, a, b);
    if (end) {
      ((C99Parser*)ctx)->add_union_type(a->lex->text());
    }
    return end;
  }
};

struct SpanUnion : public SpanMaker<SpanUnion> {
  using pattern = Seq<
    Any<SpanModifier>,
    FlatKeyword<"union">,
    Any<SpanAttribute>,
    Opt<BaseUnionTypeAdder>,
    Opt<SpanFieldList>,
    Any<SpanAttribute>,
    Opt<SpanDeclaratorList>
  >;
};

//------------------------------------------------------------------------------

struct BaseClassType : public BaseMaker<BaseClassType> {
  using pattern = Ref<&C99Parser::match_class_type>;
};

struct BaseClassTypeAdder : public BaseIdentifier {
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = BaseIdentifier::match(ctx, a, b);
    if (end) {
      ((C99Parser*)ctx)->add_class_type(a->lex->text());
    }
    return end;
  }
};

struct SpanClass : public SpanMaker<SpanClass> {
  using pattern = Seq<
    Any<SpanModifier>,
    FlatKeyword<"class">,
    Any<SpanAttribute>,
    Opt<BaseClassTypeAdder>,
    Opt<SpanFieldList>,
    Any<SpanAttribute>,
    Opt<SpanDeclaratorList>
  >;
};

//------------------------------------------------------------------------------

struct SpanTemplateParams : public SpanMaker<SpanTemplateParams> {
  using pattern =
  DelimitedList<
    FlatAtom<'<'>,
    SpanDeclaration,
    FlatAtom<','>,
    FlatAtom<'>'>
  >;
};

struct SpanTemplate : public SpanMaker<SpanTemplate> {
  using pattern = Seq<
    FlatKeyword<"template">,
    SpanTemplateParams,
    SpanClass
  >;
};

//------------------------------------------------------------------------------
// FIXME should probably have a few diffeerent versions instead of all the opts

struct BaseEnumType : public BaseMaker<BaseEnumType> {
  using pattern = Ref<&C99Parser::match_enum_type>;
};

struct BaseEnumTypeAdder : public BaseIdentifier {
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = BaseIdentifier::match(ctx, a, b);
    if (end) {
      ((C99Parser*)ctx)->add_enum_type(a->lex->text());
    }
    return end;
  }
};

struct SpanEnumerator : public SpanMaker<SpanEnumerator> {
  using pattern = Seq<
    BaseIdentifier,
    Opt<Seq<FlatAtom<'='>, SpanExpression>>
  >;
};

struct SpanEnumerators : public SpanMaker<SpanEnumerators> {
  using pattern =
  DelimitedList<
    FlatAtom<'{'>,
    SpanEnumerator,
    FlatAtom<','>,
    FlatAtom<'}'>
  >;
};

struct SpanEnum : public SpanMaker<SpanEnum> {
  using pattern = Seq<
    Any<SpanModifier>,
    FlatKeyword<"enum">,
    Opt<FlatKeyword<"class">>,
    Opt<BaseEnumTypeAdder>,
    Opt<Seq<FlatAtom<':'>, SpanTypeDecl>>,
    Opt<SpanEnumerators>,
    Opt<SpanDeclaratorList>
  >;
};

//------------------------------------------------------------------------------

struct SpanDesignation : public SpanMaker<SpanDesignation> {
  using pattern =
  Some<
    Seq<FlatAtom<'['>, BaseConstant,   FlatAtom<']'>>,
    Seq<FlatAtom<'['>, BaseIdentifier, FlatAtom<']'>>,
    Seq<FlatAtom<'.'>, BaseIdentifier>
  >;
};

struct SpanInitializerList : public SpanMaker<SpanInitializerList> {
  using pattern =
  DelimitedList<
    FlatAtom<'{'>,
    Seq<
      Opt<
        Seq<SpanDesignation, FlatAtom<'='>>,
        Seq<BaseIdentifier,  FlatAtom<':'>> // This isn't in the C grammar but compndlit-1.c uses it?
      >,
      SpanInitializer
    >,
    FlatAtom<','>,
    FlatAtom<'}'>
  >;
};

struct SpanInitializer : public SpanMaker<SpanInitializer> {
  using pattern = Oneof<
    SpanInitializerList,
    SpanExpression
  >;
};

//------------------------------------------------------------------------------

struct SpanFunctionIdentifier : public SpanMaker<SpanFunctionIdentifier> {
  using pattern = Seq<
    Any<SpanAttribute, SpanPointer>,
    Oneof<
      BaseIdentifier,
      Seq<FlatAtom<'('>, SpanFunctionIdentifier, FlatAtom<')'>>
    >
  >;
};

struct SpanFunction : public SpanMaker<SpanFunction> {
  using pattern = Seq<
    Any<
      SpanModifier,
      SpanAttribute,
      SpanSpecifier
    >,
    SpanFunctionIdentifier,
    SpanParamList,
    Opt<BaseAsmSuffix>,
    Opt<BaseKeyword<"const">>,
    // This is old-style declarations after param list
    Opt<Some<
      Seq<SpanDeclaration, FlatAtom<';'>>
    >>,
    Oneof<
      FlatAtom<';'>,
      SpanStatementCompound
    >
  >;
};

//------------------------------------------------------------------------------

struct SpanConstructor : public SpanMaker<SpanConstructor> {
  using pattern = Seq<
    BaseClassType,
    SpanParamList,
    Oneof<
      FlatAtom<';'>,
      SpanStatementCompound
    >
  >;
};

//------------------------------------------------------------------------------
// FIXME this is messy

struct SpanDeclaration : public SpanMaker<SpanDeclaration> {
  using pattern = Seq<
    Any<SpanAttribute, SpanModifier>,

    Oneof<
      Seq<
        SpanSpecifier,
        Any<SpanAttribute, SpanModifier>,
        Opt<SpanDeclaratorList>
      >,
      Seq<
        Opt<SpanSpecifier>,
        Any<SpanAttribute, SpanModifier>,
        SpanDeclaratorList
      >
    >
  >;
};

//------------------------------------------------------------------------------

struct SpanStatementCompound : public SpanMaker<SpanStatementCompound> {
  using pattern =
  DelimitedBlock<
    FlatAtom<'{'>,
    SpanStatement,
    FlatAtom<'}'>
  >;
};

//------------------------------------------------------------------------------

struct SpanStatementFor : public SpanMaker<SpanStatementFor> {
  using pattern = Seq<
    FlatKeyword<"for">,
    FlatAtom<'('>,
    // This is _not_ the same as
    // Opt<Oneof<e, x>>, FlatAtom<';'>
    Oneof<
      Seq<comma_separated<SpanExpression>, FlatAtom<';'>>,
      Seq<comma_separated<SpanDeclaration>, FlatAtom<';'>>,
      FlatAtom<';'>
    >,
    Opt<comma_separated<SpanExpression>>,
    FlatAtom<';'>,
    Opt<comma_separated<SpanExpression>>,
    FlatAtom<')'>,
    Oneof<
      SpanStatementCompound,
      SpanStatement
    >
  >;
};

//------------------------------------------------------------------------------

struct SpanStatementElse : public SpanMaker<SpanStatementElse> {
  using pattern =
  Seq<
    FlatKeyword<"else">,
    SpanStatement
  >;
};

struct SpanStatementIf : public SpanMaker<SpanStatementIf> {
  using pattern = Seq<
    FlatKeyword<"if">,

    DelimitedList<
      FlatAtom<'('>,
      SpanExpression,
      FlatAtom<','>,
      FlatAtom<')'>
    >,

    SpanStatement,
    Opt<SpanStatementElse>
  >;
};

//------------------------------------------------------------------------------

struct SpanStatementReturn : public SpanMaker<SpanStatementReturn> {
  using pattern = Seq<
    FlatKeyword<"return">,
    Opt<SpanExpression>,
    FlatAtom<';'>
  >;
};


//------------------------------------------------------------------------------

struct SpanStatementCase : public SpanMaker<SpanStatementCase> {
  using pattern = Seq<
    FlatKeyword<"case">,
    SpanExpression,
    Opt<Seq<
      // case 1...2: - this is supported by GCC?
      BaseOperator<"...">,
      SpanExpression
    >>,
    FlatAtom<':'>,
    Any<Seq<
      Not<FlatKeyword<"case">>,
      Not<FlatKeyword<"default">>,
      SpanStatement
    >>
  >;
};

struct SpanStatementDefault : public SpanMaker<SpanStatementDefault> {
  using pattern = Seq<
    FlatKeyword<"default">,
    FlatAtom<':'>,
    Any<Seq<
      Not<FlatKeyword<"case">>,
      Not<FlatKeyword<"default">>,
      SpanStatement
    >>
  >;
};

struct SpanStatementSwitch : public SpanMaker<SpanStatementSwitch> {
  using pattern = Seq<
    FlatKeyword<"switch">,
    SpanExpression,
    FlatAtom<'{'>,
    Any<
      SpanStatementCase,
      SpanStatementDefault
    >,
    FlatAtom<'}'>
  >;
};

//------------------------------------------------------------------------------

struct SpanStatementWhile : public SpanMaker<SpanStatementWhile> {
  using pattern = Seq<
    FlatKeyword<"while">,
    DelimitedList<
      FlatAtom<'('>,
      SpanExpression,
      FlatAtom<','>,
      FlatAtom<')'>
    >,
    SpanStatement
  >;
};

//------------------------------------------------------------------------------

struct SpanStatementDoWhile : public SpanMaker<SpanStatementDoWhile> {
  using pattern = Seq<
    FlatKeyword<"do">,
    SpanStatement,
    FlatKeyword<"while">,
    DelimitedList<
      FlatAtom<'('>,
      SpanExpression,
      FlatAtom<','>,
      FlatAtom<')'>
    >,
    FlatAtom<';'>
  >;
};

//------------------------------------------------------------------------------

struct BaseStatementLabel : public BaseMaker<BaseStatementLabel> {
  using pattern = Seq<
    BaseIdentifier,
    FlatAtom<':'>,
    Opt<FlatAtom<';'>>
  >;
};

//------------------------------------------------------------------------------

struct BaseStatementBreak : public BaseMaker<BaseStatementBreak> {
  using pattern = Seq<
    Keyword<"break">,
    FlatAtom<';'>
  >;
};

struct BaseStatementContinue : public BaseMaker<BaseStatementContinue> {
  using pattern = Seq<
    Keyword<"continue">,
    FlatAtom<';'>
  >;
};

//------------------------------------------------------------------------------

struct SpanAsmRef : public SpanMaker<SpanAsmRef> {
  using pattern = Seq<
    BaseAtom<LEX_STRING>,
    Opt<Seq<
      FlatAtom<'('>,
      SpanExpression,
      FlatAtom<')'>
    >>
  >;
};

struct SpanAsmRefs : public SpanMaker<SpanAsmRefs> {
  using pattern = comma_separated<SpanAsmRef>;
};

//------------------------------------------------------------------------------

struct BaseAsmQualifiers : public BaseMaker<BaseAsmQualifiers> {
  using pattern =
  Some<
    BaseKeyword<"volatile">,
    BaseKeyword<"__volatile">,
    BaseKeyword<"__volatile__">,
    BaseKeyword<"inline">,
    BaseKeyword<"goto">
  >;
};

//------------------------------------------------------------------------------

struct SpanStatementAsm : public SpanMaker<SpanStatementAsm> {
  using pattern = Seq<
    Oneof<
      FlatKeyword<"asm">,
      FlatKeyword<"__asm">,
      FlatKeyword<"__asm__">
    >,
    Opt<BaseAsmQualifiers>,
    FlatAtom<'('>,
    BaseAtom<LEX_STRING>, // assembly code
    SeqOpt<
      // output operands
      Seq<FlatAtom<':'>, Opt<SpanAsmRefs>>,
      // input operands
      Seq<FlatAtom<':'>, Opt<SpanAsmRefs>>,
      // clobbers
      Seq<FlatAtom<':'>, Opt<SpanAsmRefs>>,
      // GotoLabels
      Seq<FlatAtom<':'>, Opt<comma_separated<BaseIdentifier>>>
    >,
    FlatAtom<')'>,
    FlatAtom<';'>
  >;
};

//------------------------------------------------------------------------------

struct SpanTypedef : public SpanMaker<SpanTypedef> {
  using pattern = Seq<
    Opt<FlatKeyword<"__extension__">>,
    FlatKeyword<"typedef">,
    Oneof<
      SpanStruct,
      SpanUnion,
      SpanClass,
      SpanEnum,
      SpanDeclaration
    >
  >;

  static void extract_declarator(void* ctx, const SpanDeclarator* decl) {
    if (auto id = decl->child<BaseIdentifier>()) {
      ((C99Parser*)ctx)->add_typedef_type(id->tok_a->lex->text());
    }

    for (auto child : decl) {
      if (auto decl = child->as_a<SpanDeclarator>()) {
        extract_declarator(ctx, decl);
      }
    }
  }

  static void extract_declarator_list(void* ctx, const SpanDeclaratorList* decls) {
    if (!decls) return;
    for (auto child : decls) {
      if (auto decl = child->as_a<SpanDeclarator>()) {
        extract_declarator(ctx, decl);
      }
    }
  }

  static void extract_type(void* ctx, Token* a, Token* b) {
    auto node = a->span;

    //node->dump_tree();

    if (auto type = node->child<SpanStruct>()) {
      extract_declarator_list(ctx, type->child<SpanDeclaratorList>());
      return;
    }

    if (auto type = node->child<SpanUnion>()) {
      extract_declarator_list(ctx, type->child<SpanDeclaratorList>());
      return;
    }

    if (auto type = node->child<SpanClass>()) {
      extract_declarator_list(ctx, type->child<SpanDeclaratorList>());
      return;
    }

    if (auto type = node->child<SpanEnum>()) {
      extract_declarator_list(ctx, type->child<SpanDeclaratorList>());
      return;
    }

    if (auto type = node->child<SpanDeclaration>()) {
      extract_declarator_list(ctx, type->child<SpanDeclaratorList>());
      return;
    }

    CHECK(false);
  }

  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = SpanMaker::match(ctx, a, b);
    if (end) extract_type(ctx, a, b);
    return end;
  }
};

//------------------------------------------------------------------------------

struct SpanStatementGoto : public SpanMaker<SpanStatementGoto> {
  // pr21356.c - Spec says goto should be an identifier, GCC allows expressions
  using pattern = Seq<
    FlatKeyword<"goto">,
    SpanExpression,
    FlatAtom<';'>
  >;
};

//------------------------------------------------------------------------------

struct SpanStatement : public PatternWrapper<SpanStatement> {
  using pattern = Oneof<
    // All of these have keywords or something first
    Seq<SpanClass,   FlatAtom<';'>>,
    Seq<SpanStruct,  FlatAtom<';'>>,
    Seq<SpanUnion,   FlatAtom<';'>>,
    Seq<SpanEnum,    FlatAtom<';'>>,
    Seq<SpanTypedef, FlatAtom<';'>>,

    SpanStatementFor,
    SpanStatementIf,
    SpanStatementReturn,
    SpanStatementSwitch,
    SpanStatementDoWhile,
    SpanStatementWhile,
    SpanStatementGoto,
    SpanStatementAsm,
    SpanStatementCompound,
    BaseStatementBreak,
    BaseStatementContinue,

    // These don't - but they might confuse a keyword with an identifier...
    BaseStatementLabel,
    SpanFunction,

    // If declaration is before expression, we parse "x = 1;" as a declaration
    // because it matches a declarator (bare identifier) + initializer list :/
    Seq<comma_separated<SpanExpression>,  FlatAtom<';'>>,
    Seq<SpanDeclaration,                  FlatAtom<';'>>,

    // Extra semicolons
    FlatAtom<';'>
  >;
};

//------------------------------------------------------------------------------

struct SpanTranslationUnit : public SpanMaker<SpanTranslationUnit> {
  using pattern = Any<
    Oneof<
      Seq<SpanClass,    FlatAtom<';'>>,
      Seq<SpanStruct,   FlatAtom<';'>>,
      Seq<SpanUnion,    FlatAtom<';'>>,
      Seq<SpanEnum,     FlatAtom<';'>>,
      SpanTypedef,

      BasePreproc,
      Seq<SpanTemplate, FlatAtom<';'>>,
      SpanFunction,
      Seq<SpanDeclaration, FlatAtom<';'>>,
      FlatAtom<';'>
    >
  >;
};

//------------------------------------------------------------------------------
