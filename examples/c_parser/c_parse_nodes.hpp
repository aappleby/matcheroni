// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once

#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Utilities.hpp"

#include "examples/c_parser/c_constants.hpp"
#include "examples/c_lexer/CToken.hpp"
#include "examples/c_parser/CNode.hpp"
#include "examples/c_parser/CContext.hpp"
#include "examples/SST.hpp"

using namespace matcheroni;

template <StringParam match_name, typename P>
struct Trace2 {
  template<typename atom>
  static Span<atom> match(CContext& ctx, Span<atom> s) {
    matcheroni_assert(s.is_valid());
    if (s.is_empty()) return s.fail();

    auto text = TextSpan(s.a->a, (s.b - 1)->b);
    auto name = match_name.str_val;

    print_bar(ctx.trace_depth++, text, name, "?");
    auto end = P::match(ctx, s);
    print_bar(--ctx.trace_depth, text, name, end.is_valid() ? "OK" : "X");

    return end;
  }
};


template <StringParam match_name, typename pattern>
//using Capture2 = Trace2<match_name, Capture<match_name, pattern, CNode>>;
using Capture2 = Capture<match_name, pattern, CNode>;

//------------------------------------------------------------------------------

struct NodeAbstractDeclarator;
struct NodeClass;
struct NodeClassType;
struct NodeConstructor;
struct NodeDeclaration;
struct NodeDeclarator;
struct NodeEnum;
struct NodeEnumType;
struct NodeExpression;
struct NodeFunctionDefinition;
struct NodeInitializer;
struct NodeInitializerList;
struct NodeSpecifier;
struct NodeStatement;
struct NodeStatementCompound;
struct NodeStruct;
struct NodeStructType;
struct NodeSuffixInitializerList;
struct NodeTemplate;
struct NodeTypeDecl;
struct NodeTypedef;
struct NodeTypedefType;
struct NodeTypeName;
struct NodeUnion;
struct NodeUnionType;

//------------------------------------------------------------------------------

template <typename P>
using comma_separated = Seq<P, Any<Seq<Atom<','>, P>>, Opt<Atom<','>>>;

template <typename P>
using opt_comma_separated = Opt<comma_separated<P>>;

//------------------------------------------------------------------------------
// Matches string literals as if they were atoms. Does ___NOT___ match the
// trailing null.
// You'll need to define atom_cmp(CContext& ctx, atom& a, StringParam<N>& b) to use
// this.

template <StringParam lit>
struct Keyword : public CNode, PatternWrapper<Keyword<lit>> {
  static_assert(SST<c_keywords>::contains(lit.str_val));

  static lex_span match(CContext& ctx, lex_span s) {
    if (!s) return s.fail();
    if (ctx.compare(*s.a, LEX_KEYWORD)) return s.fail();
    /*+*/ ctx.rewind(s);
    if (ctx.compare(*s.a, lit.span())) return s.fail();
    return s.advance(1);
  }
};

template <StringParam lit>
struct Literal2 : public CNode, PatternWrapper<Literal2<lit>> {
  static lex_span match(CContext& ctx, lex_span s) {
    if (!s) return s.fail();
    if (ctx.compare(*s.a, lit.span())) return s.fail();
    return s.advance(1);
  }
};

//------------------------------------------------------------------------------

template <StringParam lit>
inline lex_span match_punct(CContext& ctx, lex_span s) {
  if (!s) return s.fail();
  if (s.len() < lit.str_len) return s.fail();

  for (auto i = 0; i < lit.str_len; i++) {
    if (ctx.compare(*s.a, LEX_PUNCT)) {
      return s.fail();
    }
    if (ctx.compare(*(s.a->a), lit.str_val[i])) {
      return s.fail();
    }
    s.advance(1);
  }

  return s;
}

//------------------------------------------------------------------------------

template <auto P>
struct NodeAtom : public CNode, PatternWrapper<NodeAtom<P>> {
  using pattern = Atom<P>;
};

template <StringParam lit>
struct NodeKeyword : public CNode, PatternWrapper<NodeKeyword<lit>> {
  using pattern = Keyword<lit>;
};

template <StringParam lit>
struct NodeLiteral : public CNode, PatternWrapper<NodeLiteral<lit>> {
  using pattern = Literal2<lit>;
};

//------------------------------------------------------------------------------
// Our builtin types are any sequence of prefixes followed by a builtin type

struct NodeBuiltinType : public CNode, PatternWrapper<NodeBuiltinType> {
  using match_prefix = Ref<&CContext::match_builtin_type_prefix>;
  using match_base = Ref<&CContext::match_builtin_type_base>;
  using match_suffix = Ref<&CContext::match_builtin_type_suffix>;

  // clang-format off
  using pattern =
  Seq<
    Any<
      Seq<
        Capture2<"prefix", match_prefix>,
        And<match_base>
      >
    >,
    Capture2<"base_type", match_base>,
    Opt<Capture2<"suffix", match_suffix>>
  >;
  // clang-format on
};

struct NodeTypedefType : public CNode, PatternWrapper<NodeTypedefType> {
  using pattern = Ref<&CContext::match_typedef_type>;
};

//------------------------------------------------------------------------------
// Excluding builtins and typedefs from being identifiers changes the total
// number of parse nodes, but why?

// - Because "uint8_t *x = 5" gets misparsed as an expression if uint8_t matches
// as an identifier

struct NodeIdentifier : public CNode, PatternWrapper<NodeIdentifier> {
  using pattern =
  Seq<
    Not<NodeBuiltinType>,
    Not<NodeTypedefType>,
    Atom<LEX_IDENTIFIER>
  >;
};

//------------------------------------------------------------------------------

struct NodePreproc : public CNode, PatternWrapper<NodePreproc> {
  using pattern = Atom<LEX_PREPROC>;
};

//------------------------------------------------------------------------------

struct NodeConstant : public CNode, PatternWrapper<NodeConstant> {
  using pattern =
  Oneof<
    Capture2<"float",  Atom<LEX_FLOAT>>,
    Capture2<"int",    Atom<LEX_INT>>,
    Capture2<"char",   Atom<LEX_CHAR>>,
    Capture2<"string", Some<Atom<LEX_STRING>>>
  >;
};

//------------------------------------------------------------------------------

template <StringParam lit>
struct NodePrefixOp : public CNode, PatternWrapper<NodePrefixOp<lit>> {
  NodePrefixOp() {
    precedence = prefix_precedence(lit.str_val);
    assoc = prefix_assoc(lit.str_val);
  }

  using pattern = Ref<match_punct<lit>>;
};

//------------------------------------------------------------------------------

template <StringParam lit>
struct NodeBinaryOp : public CNode, PatternWrapper<NodeBinaryOp<lit>> {
  NodeBinaryOp() {
    precedence = binary_precedence(lit.str_val);
    assoc = binary_assoc(lit.str_val);
  }

  using pattern = Ref<match_punct<lit>>;
};

//------------------------------------------------------------------------------

template <StringParam lit>
struct NodeSuffixOp : public CNode, PatternWrapper<NodeSuffixOp<lit>> {
  NodeSuffixOp() {
    precedence = suffix_precedence(lit.str_val);
    assoc = suffix_assoc(lit.str_val);
  }

  using pattern = Ref<match_punct<lit>>;
};


//------------------------------------------------------------------------------

struct NodeQualifier : public CNode, PatternWrapper<NodeQualifier> {
  static lex_span match(CContext& ctx, lex_span s) {
    matcheroni_assert(s.is_valid());
    TextSpan span = *(s.a);
    if (SST<qualifiers>::match(span.a, span.b)) {
      return s.advance(1);
    }
    else {
      return s.fail();
    }
  }
};


//------------------------------------------------------------------------------

struct NodeAsmSuffix : public CNode, PatternWrapper<NodeAsmSuffix> {
  using pattern =
  Seq<
    Oneof<
      Keyword<"asm">,
      Keyword<"__asm">,
      Keyword<"__asm__">
    >,
    Atom<'('>,
    Capture2<"code", Some<NodeAtom<LEX_STRING>>>,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------


struct NodeAccessSpecifier : public CNode, PatternWrapper<NodeAccessSpecifier> {
  using pattern =
  Seq<
    Oneof<
      Literal2<"public">,
      Literal2<"private">
    >,
    Atom<':'>
  >;
};

//------------------------------------------------------------------------------
// (6.5.4) cast-expression:
//   unary-expression
//   ( type-name ) cast-expression

struct NodeParenType : public CNode, public PatternWrapper<NodeParenType> {
  using pattern = Seq<Atom<'('>, NodeTypeName, Atom<')'>>;
};

struct NodePrefixCast : public CNode, public PatternWrapper<NodePrefixCast> {
  NodePrefixCast() {
    precedence = 3;
    assoc = -2;
  }

  using pattern = Seq<Atom<'('>, NodeTypeName, Atom<')'>>;
};


//------------------------------------------------------------------------------

struct NodeExpressionParen : public CNode, PatternWrapper<NodeExpressionParen> {
  using pattern =
      DelimitedList<Atom<'('>, NodeExpression, Atom<','>, Atom<')'>>;
};

struct NodeSuffixParen : public CNode, PatternWrapper<NodeSuffixParen> {
  NodeSuffixParen() {
    precedence = 2;
    assoc = 2;
  }

  using pattern =
  DelimitedList<
    Atom<'('>,
    Capture2<"expression", NodeExpression>,
    Atom<','>,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeExpressionBraces : public CNode, PatternWrapper<NodeExpressionBraces> {
  using pattern =
      DelimitedList<Atom<'{'>, NodeExpression, Atom<','>, Atom<'}'>>;
};

struct NodeSuffixBraces : public CNode, PatternWrapper<NodeSuffixBraces> {
  NodeSuffixBraces() {
    precedence = 2;
    assoc = 2;
  }

  using pattern =
      DelimitedList<Atom<'{'>, NodeExpression, Atom<','>, Atom<'}'>>;
};


//------------------------------------------------------------------------------

struct NodeSuffixSubscript : public CNode, PatternWrapper<NodeSuffixSubscript> {
  NodeSuffixSubscript() {
    precedence = 2;
    assoc = 2;
  }

  using pattern =
      DelimitedList<Atom<'['>, NodeExpression, Atom<','>, Atom<']'>>;
};

//------------------------------------------------------------------------------
// This is a weird ({...}) thing that GCC supports

struct NodeExpressionGccCompound : public CNode, PatternWrapper<NodeExpressionGccCompound> {
  using pattern =
  Seq<
    Opt<Keyword<"__extension__">>,
    Atom<'('>,
    NodeStatementCompound,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeExpressionTernary : public CNode {};
struct NodeExpressionBinary : public CNode {};
struct NodeExpressionPrefix : public CNode {};
struct NodeExpressionSuffix : public CNode {};

struct NodeExpressionSizeof : public CNode, PatternWrapper<NodeExpressionSizeof> {
  using pattern =
  Seq<
    Keyword<"sizeof">,
    Capture2<
      "value",
      Oneof<
        NodeParenType,
        NodeExpressionParen,
        NodeExpression
      >
    >
  >;
};

struct NodeExpressionAlignof : public CNode, PatternWrapper<NodeExpressionAlignof> {
  using pattern =
  Seq<
    Keyword<"__alignof__">,
    Capture2<
      "value",
      Oneof<
        NodeParenType,
        NodeExpressionParen
      >
    >
  >;
};

struct NodeExpressionOffsetof : public CNode, PatternWrapper<NodeExpressionOffsetof> {
  using pattern =
  Seq<
    Oneof<
      Keyword<"offsetof">,
      Keyword<"__builtin_offsetof">
    >,
    Atom<'('>,
    Capture2<"name", NodeTypeName>,
    Atom<','>,
    Capture2<"expression", NodeExpression>,
    Atom<')'>
  >;
};

//----------------------------------------

template <StringParam lit>
struct NodePrefixKeyword : public CNode, PatternWrapper<NodePrefixKeyword<lit>> {
  NodePrefixKeyword() {
    precedence = 3;
    assoc = -2;
  }

  using pattern = Keyword<lit>;
};

// clang-format off
using ExpressionPrefixOp =
Oneof<
  Capture2<"cast",      NodePrefixCast>,
  Capture2<"extension", NodePrefixKeyword<"__extension__">>,
  Capture2<"real",      NodePrefixKeyword<"__real">>,
  Capture2<"real",      NodePrefixKeyword<"__real__">>,
  Capture2<"imag",      NodePrefixKeyword<"__imag">>,
  Capture2<"imag",      NodePrefixKeyword<"__imag__">>,
  Capture2<"preinc",    NodePrefixOp<"++">>,
  Capture2<"predec",    NodePrefixOp<"--">>,
  Capture2<"preplus",   NodePrefixOp<"+">>,
  Capture2<"preminus",  NodePrefixOp<"-">>,
  Capture2<"prebang",   NodePrefixOp<"!">>,
  Capture2<"pretilde",  NodePrefixOp<"~">>,
  Capture2<"prestar",   NodePrefixOp<"*">>,
  Capture2<"preamp",    NodePrefixOp<"&">>
>;
// clang-format on

//----------------------------------------

// clang-format off
using ExpressionCore =
Oneof<
  Capture2<"sizeof",       NodeExpressionSizeof>,
  Capture2<"alignof",      NodeExpressionAlignof>,
  Capture2<"offsetof",     NodeExpressionOffsetof>,
  Capture2<"gcc_compound", NodeExpressionGccCompound>,
  Capture2<"paren",        NodeExpressionParen>,
  Capture2<"init",         NodeInitializerList>,
  Capture2<"braces",       NodeExpressionBraces>,
  Capture2<"identifier",   NodeIdentifier>,
  Capture2<"constant",     NodeConstant>
>;
// clang-format on

//----------------------------------------

// clang-format off
using ExpressionSuffixOp =
Oneof<
  Capture2<"initializer", NodeSuffixInitializerList>,  // must be before NodeSuffixBraces
  Capture2<"braces",      NodeSuffixBraces>,
  Capture2<"paren",       NodeSuffixParen>,
  Capture2<"subscript",   NodeSuffixSubscript>,
  Capture2<"postinc",     NodeSuffixOp<"++">>,
  Capture2<"postdec",     NodeSuffixOp<"--">>
>;
// clang-format on

//----------------------------------------

// clang-format off
using unit_pattern =
Seq<
  Any<
    Capture2<"prefix", ExpressionPrefixOp>
  >,
  ExpressionCore,
  Any<
    Capture2<"suffix", ExpressionSuffixOp>
  >
>;
// clang-format on

//----------------------------------------

struct NodeTernaryOp : public CNode, PatternWrapper<NodeTernaryOp> {
  NodeTernaryOp() {
    precedence = 16;
    assoc = -1;
  }

  using pattern =
  Seq<
    NodeBinaryOp<"?">,
    Opt<
      Capture2<"then", comma_separated<NodeExpression>>
    >,
    NodeBinaryOp<":">
  >;
};

//----------------------------------------

struct NodeExpression : public CNode, PatternWrapper<NodeExpression> {
  static lex_span match_binary_op(CContext& ctx, lex_span s) {
    matcheroni_assert(s);

    if (ctx.compare(*s.a, LEX_PUNCT)) {
      return s.fail();
    }

    // clang-format off
    switch (s.a->a[0]) {
      case '+':
        return Oneof<NodeBinaryOp<"+=">, NodeBinaryOp<"+">>::match(ctx, s);
      case '-':
        return Oneof<NodeBinaryOp<"->*">, NodeBinaryOp<"->">, NodeBinaryOp<"-=">, NodeBinaryOp<"-">>::match(ctx, s);
      case '*':
        return Oneof<NodeBinaryOp<"*=">, NodeBinaryOp<"*">>::match(ctx, s);
      case '/':
        return Oneof<NodeBinaryOp<"/=">, NodeBinaryOp<"/">>::match(ctx, s);
      case '=':
        return Oneof<NodeBinaryOp<"==">, NodeBinaryOp<"=">>::match(ctx, s);
      case '<':
        return Oneof<NodeBinaryOp<"<<=">, NodeBinaryOp<"<=>">, NodeBinaryOp<"<=">, NodeBinaryOp<"<<">, NodeBinaryOp<"<">>::match(ctx, s);
      case '>':
        return Oneof<NodeBinaryOp<">>=">, NodeBinaryOp<">=">, NodeBinaryOp<">>">, NodeBinaryOp<">">>::match(ctx, s);
      case '!':
        return NodeBinaryOp<"!=">::match(ctx, s);
      case '&':
        return Oneof<NodeBinaryOp<"&&">, NodeBinaryOp<"&=">, NodeBinaryOp<"&">>::match(ctx, s);
      case '|':
        return Oneof<NodeBinaryOp<"||">, NodeBinaryOp<"|=">, NodeBinaryOp<"|">>::match(ctx, s);
      case '^':
        return Oneof<NodeBinaryOp<"^=">, NodeBinaryOp<"^">>::match(ctx, s);
      case '%':
        return Oneof<NodeBinaryOp<"%=">, NodeBinaryOp<"%">>::match(ctx, s);
      case '.':
        return Oneof<NodeBinaryOp<".*">, NodeBinaryOp<".">>::match(ctx, s);
      case '?':
        return NodeTernaryOp::match(ctx, s);

        // FIXME this is only for C++, and
        // case ':': return NodeBinaryOp<"::">::match(ctx, s);
        // default:  return nullptr;
    }
    // clang-format on

    return s.fail();
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

  static lex_span match2(CContext& ctx, lex_span s) {

    using pattern =
        Seq<unit_pattern, Any<Seq<Ref<match_binary_op>, unit_pattern>>>;

    auto end = pattern::match(ctx, s);
    if (!end.is_valid()) {
      return s.fail();
    }

    //auto tok_a = s.a;
    //auto tok_b = end.a;

    /*
    while (0) {
      {
        auto c = tok_a;
        while (c && c < tok_b) {
          c->span->dump_tree(0, 1);
          c = c->step_right();
        }
        printf("\n");
      }

      auto c = tok_a;
      while (c && c->span->precedence && c < tok_b) {
        c = c->step_right();
      }

      c->dump_token();

      // ran out of units?
      if (c->span->precedence) break;

      auto l = c - 1;
      if (l && l >= tok_a) {
        if (l->span->assoc == -2) {
          auto node = new NodeExpressionPrefix();
          node->init_node(ctx, l, c, l->span, c->span);
          continue;
        }
      }

      break;
    }
    */

    // dump_tree(n, 0, 0);

    // Fold up as many nodes based on precedence as we can
#if 0
    while(1) {
      ParseNode*    na = nullptr;
      NodeOpBinary* ox = nullptr;
      ParseNode*    nb = nullptr;
      NodeOpBinary* oy = nullptr;
      ParseNode*    nc = nullptr;

      nc = (cursor - 1)->span->as_a<ParseNode>();
      oy = nc ? nc->left_neighbor()->as_a<NodeOpBinary>()   : nullptr;
      nb = oy ? oy->left_neighbor()->as_a<ParseNode>() : nullptr;
      ox = nb ? nb->left_neighbor()->as_a<NodeOpBinary>()   : nullptr;
      na = ox ? ox->left_neighbor()->as_a<ParseNode>() : nullptr;




      if (!na || !ox || !nb || !oy || !nc) break;

      if (na->tok_b() < a) break;

      if (ox->precedence < oy->precedence) {
        // Left-associate because right operator is "lower" precedence.
        // "a * b + c" -> "(a * b) + c"
        auto node = new NodeExpressionBinary();
        node->init(na->tok_a(), nb->tok_b());
      }
      else if (ox->precedence == oy->precedence) {
        matcheroni_assert(ox->assoc == oy->assoc);

        if (ox->assoc == 1) {
          // Left to right
          // "a + b - c" -> "(a + b) - c"
          auto node = new NodeExpressionBinary();
          node->init(na->tok_a(), nb->tok_b());
        }
        else if (ox->assoc == -1) {
          // Right to left
          // "a = b = c" -> "a = (b = c)"
          auto node = new NodeExpressionBinary();
          node->init(nb->tok_a(), nc->tok_b());
        }
        else {
          matcheroni_assert(false);
        }
      }
      else {
        break;
      }
    }
#endif

    // Any binary operators left on the tokens are in increasing-precedence
    // order, but since there are no more operators we can just fold them up
    // right-to-left
#if 0
    while(1) {
      ParseNode*    nb = nullptr;
      NodeOpBinary* oy = nullptr;
      ParseNode*    nc = nullptr;

      nc = (cursor - 1)->span->as_a<ParseNode>();
      oy = nc ? nc->left_neighbor()->as_a<NodeOpBinary>()   : nullptr;
      nb = oy ? oy->left_neighbor()->as_a<ParseNode>() : nullptr;

      if (!nb || !oy || !nc) break;
      if (nb->tok_b() < a) break;

      auto node = new NodeExpressionBinary();
      node->init(nb->tok_a(), nc->tok_b());
    }
#endif

#if 0
    if (auto end = SpanTernaryOp::match(ctx, cursor, b)) {
      auto node = new NodeExpressionTernary();
      node->init(a, end - 1);
      cursor = end;
    }
#endif

#if 0
    {
      printf("---EXPRESSION---\n");
      const CToken* c = a;
      while(1) {
        //c->dump_token();
        if (auto s = c->span) {
          dump_tree(s, 1, 0);
        }
        if (auto end = c->span->tok_b()) {
          c = end + 1;
        }
        else {
          c++;
        }
        if (c == tok_b) break;
      }
      printf("----------------\n");
    }
#endif

    return end;
  }

  //----------------------------------------

  static lex_span match(CContext& ctx, lex_span s) {
    //print_trace_start<NodeExpression, CToken>(a);
    auto end = match2(ctx, s);
    if (end) {
      //auto node = new NodeExpression();
      //node->init_node(ctx, a, end - 1, a->span, (end - 1)->span);
    }
    //print_trace_end<NodeExpression, CToken>(a, end);
    return end;
  }
};

//------------------------------------------------------------------------------
// 20010911-1.c - Attribute can be empty

struct NodeAttribute : public CNode, public PatternWrapper<NodeAttribute> {
  // clang-format off
  using pattern =
  Seq<
    Oneof<
      Keyword<"__attribute__">,
      Keyword<"__attribute">
    >,
    DelimitedList<
      Seq<Atom<'('>, Atom<'('>>,
      Oneof<
        Capture2<"expression", NodeExpression>,
        Capture2<"keyword", Atom<LEX_KEYWORD>>
      >,
      Atom<','>,
      Seq<Atom<')'>, Atom<')'>>
    >
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

struct NodeAlignas : public CNode, public PatternWrapper<NodeAlignas> {
  // clang-format off
  using pattern =
  Seq<
    Keyword<"_Alignas">,
    Atom<'('>,
    Capture2<
      "value",
      Oneof<
        NodeTypeDecl,
        NodeConstant
      >
    >,
    Atom<')'>
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

struct NodeDeclspec : public CNode, public PatternWrapper<NodeDeclspec> {
  // clang-format off
  using pattern =
  Seq<
    Keyword<"__declspec">,
    Atom<'('>,
    Capture2<"identifier", NodeIdentifier>,
    Atom<')'>
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

struct NodeModifier : public PatternWrapper<NodeModifier> {
  // This is the slowest matcher in the app, why?
  using pattern =
  Oneof<
    Capture2<"alignas",   NodeAlignas>,
    Capture2<"declspec",  NodeDeclspec>,
    Capture2<"attribute", NodeAttribute>,
    Capture2<"qualifier", NodeQualifier>
  >;
};

//------------------------------------------------------------------------------

struct NodeTypeDecl : public CNode, public PatternWrapper<NodeTypeDecl> {
  using pattern =
  Seq<
    Any<
      Capture2<"modifier", NodeModifier>
    >,
    Capture2<"specifier", NodeSpecifier>,
    Opt<
      Capture2<"abstract_decl", NodeAbstractDeclarator>
    >
  >;
};

//------------------------------------------------------------------------------

struct NodePointer : public CNode, public PatternWrapper<NodePointer> {
  using pattern =
  Seq<
    Literal2<"*">,
    Any<
      Literal2<"*">,
      Capture2<"modifier", NodeModifier>
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeEllipsis : public CNode, public PatternWrapper<NodeEllipsis> {
  using pattern = Ref<match_punct<"...">>;
};

//------------------------------------------------------------------------------

struct NodeParam : public CNode, public PatternWrapper<NodeParam> {
  using pattern =
  Oneof<
    Capture2<"ellipsis", NodeEllipsis>,
    Seq<
      Any<Capture2<"modifier", NodeModifier>>,
      Capture2<"specifier", NodeSpecifier>,
      Any<Capture2<"modifier", NodeModifier>>,
      Capture2<"decl", Opt<NodeDeclarator, NodeAbstractDeclarator>>
    >,
    Capture2<"identifier", NodeIdentifier>
  >;
};

//------------------------------------------------------------------------------

struct NodeParamList : public CNode, public PatternWrapper<NodeParamList> {
  using pattern =
  DelimitedList<
    Atom<'('>,
    Capture2<"param", NodeParam>,
    Atom<','>,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeArraySuffix : public CNode, public PatternWrapper<NodeArraySuffix> {
  // clang-format off
  using pattern =
  Oneof<
    Seq<Atom<'['>, Any<NodeModifier>, Opt<NodeExpression>, Atom<']'>>,
    Seq<Atom<'['>, NodeKeyword<"static">, Any<NodeModifier>, NodeExpression, Atom<']'>>,
    Seq<Atom<'['>, Any<NodeModifier>, NodeKeyword<"static">, NodeExpression, Atom<']'>>,
    Seq<Atom<'['>, Any<NodeModifier>, Literal2<"*">, Atom<']'>>
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

struct NodeTemplateArgs : public CNode, public PatternWrapper<NodeTemplateArgs> {
  using pattern =
      DelimitedList<Atom<'<'>, NodeExpression, Atom<','>, Atom<'>'>>;
};

//------------------------------------------------------------------------------

struct NodeAtomicType : public CNode, public PatternWrapper<NodeAtomicType> {
  using pattern = Seq<Keyword<"_Atomic">, Atom<'('>, NodeTypeDecl, Atom<')'>>;
};

//------------------------------------------------------------------------------

struct NodeSpecifier : public CNode, public PatternWrapper<NodeSpecifier> {
  // clang-format off
  using pattern =
  Seq<
    Oneof<
      // These have to be NodeIdentifier because "void foo(struct S);"
      // is valid even without the definition of S.
      Seq<NodeLiteral<"class">, Oneof<NodeIdentifier, NodeTypedefType>>,
      Seq<Keyword<"union">,     Oneof<NodeIdentifier, NodeTypedefType>>,
      Seq<Keyword<"struct">,    Oneof<NodeIdentifier, NodeTypedefType>>,
      Seq<Keyword<"enum">,      Oneof<NodeIdentifier, NodeTypedefType>>,

      /*
      // If this was C++, we would also need to match these directly
      BaseClassType,
      BaseStructType,
      NodeUnionType,
      NodeEnumType,
      */

      NodeBuiltinType,
      NodeTypedefType,
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
  // clang-format on
};

//------------------------------------------------------------------------------
// (6.7.6) type-name:
//   specifier-qualifier-list abstract-declaratoropt

struct NodeTypeName : public CNode, public PatternWrapper<NodeTypeName> {
  using pattern =
      Seq<Some<NodeSpecifier, NodeModifier>, Opt<NodeAbstractDeclarator>>;
};

//------------------------------------------------------------------------------
// Spec says the bit size can be any constant expression, but can we use just a
// constant or a paren-expression?

// (6.7.2.1) struct-declarator:
//   declarator
//   declaratoropt : constant-expression

struct NodeBitSuffix : public CNode, public PatternWrapper<NodeBitSuffix> {
  using pattern = Seq<Atom<':'>, NodeExpression>;
};

//------------------------------------------------------------------------------

struct NodeAbstractDeclarator : public CNode,
                                public PatternWrapper<NodeAbstractDeclarator> {
  using pattern =
  Seq<
    Opt<NodePointer>,
    Opt<
      Seq<
        Atom<'('>,
        NodeAbstractDeclarator,
        Atom<')'>
      >
    >,
    Any<
      NodeAttribute,
      NodeArraySuffix,
      NodeParamList
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeDeclarator : public CNode, public PatternWrapper<NodeDeclarator> {
  using pattern =
  Seq<
    Any<
      Capture2<"attribute", NodeAttribute>,
      Capture2<"modifier",  NodeModifier>,
      Capture2<"pointer",   NodePointer>
    >,
    Oneof<
      Capture2<"identifier", NodeIdentifier>,
      Capture2<"declarator",
        Seq<
          Atom<'('>,
          NodeDeclarator,
          Atom<')'>
        >
      >
    >,
    Any<
      Capture2<"asm_suffix",   NodeAsmSuffix>,
      Capture2<"bit_suffix",   NodeBitSuffix>,
      Capture2<"attribute",    NodeAttribute>,
      Capture2<"array_suffix", NodeArraySuffix>,
      Capture2<"param_list",   NodeParamList>
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeDeclaratorList : public CNode,
                            public PatternWrapper<NodeDeclaratorList> {
  // clang-format off
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
      Opt<
        Seq<
          Atom<'='>,
          NodeInitializer
        >
      >
    >
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

struct NodeField : public PatternWrapper<NodeField> {
  // clang-format off
  using pattern =
  Oneof<
    Atom<';'>,
    NodeAccessSpecifier,
    NodeConstructor,
    NodeFunctionDefinition,
    NodeStruct,
    NodeUnion,
    NodeTemplate,
    NodeClass,
    NodeEnum,
    NodeDeclaration
  >;
  // clang-format on
};

struct NodeFieldList : public CNode, public PatternWrapper<NodeFieldList> {
  using pattern = DelimitedBlock<Atom<'{'>, NodeField, Atom<'}'>>;
};

//------------------------------------------------------------------------------

struct NodeNamespace : public CNode, public PatternWrapper<NodeNamespace> {
  using pattern =
      Seq<Keyword<"namespace">, Opt<NodeIdentifier>, Opt<NodeFieldList>>;
};

//------------------------------------------------------------------------------

struct NodeStructType : public CNode, public PatternWrapper<NodeStructType> {
  using pattern = Ref<&CContext::match_struct_type>;
};

struct NodeStructTypeAdder : public NodeIdentifier {
  static lex_span match(CContext& ctx, lex_span s) {
    if (auto end = NodeIdentifier::match(ctx, s)) {
      ctx.add_struct_type(s.a);
      return end;
    } else if (auto end = NodeTypedefType::match(ctx, s)) {
      // Already typedef'd
      return end;
    } else {
      return s.fail();
    }
  }
};

struct NodeStruct : public CNode, public PatternWrapper<NodeStruct> {
  // clang-format off
  using pattern =
  Seq<
    Any<NodeModifier>,
    Keyword<"struct">,
    Any<NodeAttribute>,  // This has to be here, there are a lot of struct __attrib__() foo {};
    Opt<NodeStructTypeAdder>,
    Opt<NodeFieldList>,
    Any<NodeAttribute>,
    Opt<NodeDeclaratorList>
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

struct NodeUnionType : public CNode {
  static lex_span match(CContext& ctx, lex_span s) {
    return ctx.match_union_type(s);
  }
};

struct NodeUnionTypeAdder : public NodeIdentifier {
  static lex_span match(CContext& ctx, lex_span s) {
    if (auto end = NodeIdentifier::match(ctx, s)) {
      ctx.add_union_type(s.a);
      return end;
    } else if (auto end = NodeTypedefType::match(ctx, s)) {
      // Already typedef'd
      return end;
    } else {
      return s.fail();
    }
  }
};

struct NodeUnion : public CNode, public PatternWrapper<NodeUnion> {
  // clang-format off
  using pattern =
  Seq<
    Any<NodeModifier>,
    Keyword<"union">,
    Any<NodeAttribute>,
    Opt<NodeUnionTypeAdder>,
    Opt<NodeFieldList>,
    Any<NodeAttribute>,
    Opt<NodeDeclaratorList>
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

struct NodeClassType : public CNode, public PatternWrapper<NodeClassType> {
  using pattern = Ref<&CContext::match_class_type>;
};

struct NodeClassTypeAdder : public NodeIdentifier {
  static lex_span match(CContext& ctx, lex_span s) {
    if (auto end = NodeIdentifier::match(ctx, s)) {
      ctx.add_class_type(s.a);
      return end;
    } else if (auto end = NodeTypedefType::match(ctx, s)) {
      // Already typedef'd
      return end;
    } else {
      return s.fail();
    }
  }
};

struct NodeClass : public CNode, public PatternWrapper<NodeClass> {
  // clang-format off
  using pattern =
  Seq<
    Any<NodeModifier>,
    NodeLiteral<"class">,
    Any<Capture2<"attribute", NodeAttribute>>,
    Opt<NodeClassTypeAdder>,
    Opt<Capture2<"body", NodeFieldList>>,
    Any<Capture2<"attribute", NodeAttribute>>,
    Opt<Capture2<"decls", NodeDeclaratorList>>
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

struct NodeTemplateParams : public CNode,
                            public PatternWrapper<NodeTemplateParams> {
  using pattern =
  DelimitedList<
    Atom<'<'>,
    Capture2<"param", NodeDeclaration>,
    Atom<','>,
    Atom<'>'>
  >;
};

struct NodeTemplate : public CNode, public PatternWrapper<NodeTemplate> {
  using pattern =
  Seq<
    NodeLiteral<"template">,
    Capture2<"params", NodeTemplateParams>,
    Capture2<"class", NodeClass>
  >;
};

//------------------------------------------------------------------------------
// FIXME should probably have a few diffeerent versions instead of all the opts

struct NodeEnumType : public CNode, public PatternWrapper<NodeEnumType> {
  using pattern = Ref<&CContext::match_enum_type>;
};

struct NodeEnumTypeAdder : public NodeIdentifier {
  static lex_span match(CContext& ctx, lex_span s) {
    if (auto end = NodeIdentifier::match(ctx, s)) {
      ctx.add_enum_type(s.a);
      return end;
    } else if (auto end = NodeTypedefType::match(ctx, s)) {
      // Already typedef'd
      return end;
    } else {
      return s.fail();
    }
  }
};

struct NodeEnumerator : public CNode, public PatternWrapper<NodeEnumerator> {
  using pattern =
  Seq<
    Capture2<"name", NodeIdentifier>,
    Opt<
      Seq<
        Atom<'='>,
        Capture2<"value", NodeExpression>
      >
    >
  >;
};

struct NodeEnumerators : public CNode, public PatternWrapper<NodeEnumerators> {
  using pattern =
  DelimitedList<
    Atom<'{'>,
    Capture2<"enumerator", NodeEnumerator>,
    Atom<','>,
    Atom<'}'>
  >;
};

struct NodeEnum : public CNode, public PatternWrapper<NodeEnum> {
  // clang-format off
  using pattern =
  Seq<
    Any<NodeModifier>,
    Keyword<"enum">,
    Opt<NodeLiteral<"class">>,
    Opt<NodeEnumTypeAdder>,
    Opt<Seq<Atom<':'>, Capture2<"type", NodeTypeDecl>>>,
    Opt<Capture2<"body", NodeEnumerators>>,
    Opt<Capture2<"decls", NodeDeclaratorList>>
  >;
};

//------------------------------------------------------------------------------

struct NodeDesignation : public CNode, public PatternWrapper<NodeDesignation> {
  // clang-format off
  using pattern =
  Some<
    Seq<Atom<'['>, NodeConstant, Atom<']'>>,
    Seq<Atom<'['>, NodeIdentifier, Atom<']'>>,
    Seq<Atom<'.'>, NodeIdentifier>
  >;
  // clang-format on
};

struct NodeInitializerList : public CNode, public PatternWrapper<NodeInitializerList> {
  using pattern = DelimitedList<
      Atom<'{'>,
      Seq<Opt<Seq<NodeDesignation, Atom<'='>>,
              Seq<NodeIdentifier, Atom<':'>>  // This isn't in the C grammar but
                                              // compndlit-1.c uses it?
              >,
          NodeInitializer>,
      Atom<','>, Atom<'}'>>;
};

struct NodeSuffixInitializerList : public CNode, public PatternWrapper<NodeSuffixInitializerList> {
  NodeSuffixInitializerList() {
    precedence = 2;
    assoc = 2;
  }

  // clang-format off
  using pattern =
  DelimitedList<
    Atom<'{'>,
    Seq<
      Opt<
        Seq<NodeDesignation, Atom<'='>>,
        Seq<NodeIdentifier, Atom<':'>>  // This isn't in the C grammar but compndlit-1.c uses it?
      >,
      NodeInitializer
    >,
    Atom<','>,
    Atom<'}'>
  >;
  // clang-format on
};

struct NodeInitializer : public CNode, public PatternWrapper<NodeInitializer> {
  using pattern = Oneof<NodeInitializerList, NodeExpression>;
};

//------------------------------------------------------------------------------

struct NodeFunctionIdentifier : public CNode,
                                public PatternWrapper<NodeFunctionIdentifier> {
  // clang-format off
  using pattern =
  Seq<
    Any<NodeAttribute, NodePointer>,
    Oneof<
      NodeIdentifier,
      Seq<Atom<'('>, NodeFunctionIdentifier, Atom<')'>>
    >
  >;
  // clang-format on
};

// function-definition:
//     declaration-specifiers declarator declaration-listopt compound-statement

struct NodeFunctionDefinition : public CNode,
                                public PatternWrapper<NodeFunctionDefinition> {
  // clang-format off
  using pattern =
  Seq<
    Any<NodeModifier>,
    Capture2<"func_return_type", Opt<NodeSpecifier>>,
    Any<NodeModifier>,
    Capture2<"func_identifier", NodeFunctionIdentifier>,
    Capture2<"func_params", NodeParamList>,
    Any<NodeModifier>,
    Opt<NodeAsmSuffix>,
    Opt<NodeKeyword<"const">>,
    // This is old-style declarations after param list
    Opt<Some<Seq<NodeDeclaration, Atom<';'>>>>,
    Capture2<"func_body", NodeStatementCompound>
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

struct NodeConstructor : public CNode, public PatternWrapper<NodeConstructor> {
  // clang-format off
  using pattern =
  Seq<
    NodeClassType,
    Capture2<"params", NodeParamList>,
    Oneof<
      Atom<';'>,
      Capture2<"body", NodeStatementCompound>
    >
  >;
  // clang-format om
};

//------------------------------------------------------------------------------
// FIXME this is messy

struct NodeDeclaration : public CNode, public PatternWrapper<NodeDeclaration> {
  // clang-format off
  using pattern =
  Seq<
    Any<
      Capture2<"attribute", NodeAttribute>,
      Capture2<"modifier", NodeModifier>
    >,
    Oneof<
      Seq<
        Capture2<"specifier", NodeSpecifier>,
        Any<
          Capture2<"attribute", NodeAttribute>,
          Capture2<"modifier", NodeModifier>
        >,
        Opt<
          Capture2<"decls", NodeDeclaratorList>
        >
      >,
      Seq<
        Opt<Capture2<"specifier", NodeSpecifier>>,
        Any<
          Capture2<"attribute", NodeAttribute>,
          Capture2<"modifier", NodeModifier>
        >,
        Capture2<"decls", NodeDeclaratorList>
      >
    >
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

template <typename P>
struct PushPopScope {
  static lex_span match(CContext& ctx, lex_span s) {
    ctx.push_scope();
    auto end = P::match(ctx, s);
    ctx.pop_scope();
    return end;
  }
};

struct NodeStatementCompound : public CNode, public PatternWrapper<NodeStatementCompound> {
  using pattern =
  PushPopScope<
    DelimitedBlock<
      Atom<'{'>,
      Capture2<"statement", NodeStatement>,
      Atom<'}'>
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementFor : public CNode, public PatternWrapper<NodeStatementFor> {
  // clang-format off
  using pattern =
  Seq<
    Keyword<"for">,
    Atom<'('>,
    Capture2<
      "init",
      // This is _not_ the same as
      // Opt<Oneof<e, x>>, Atom<';'>
      Oneof<
        Seq<comma_separated<NodeExpression>,  Atom<';'>>,
        Seq<comma_separated<NodeDeclaration>, Atom<';'>>,
        Atom<';'>
      >
    >,
    Capture2<"condition", Opt<comma_separated<NodeExpression>>>,
    Atom<';'>,
    Capture2<"step", Opt<comma_separated<NodeExpression>>>,
    Atom<')'>,
    Oneof<NodeStatementCompound, NodeStatement>
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

struct NodeStatementElse : public CNode, public PatternWrapper<NodeStatementElse> {
  using pattern = Seq<Keyword<"else">, NodeStatement>;
};

struct NodeStatementIf : public CNode, public PatternWrapper<NodeStatementIf> {
  using pattern =
  Seq<
    Keyword<"if">,
    Capture2<
      "condition",
      DelimitedList<Atom<'('>, NodeExpression, Atom<','>, Atom<')'>>
    >,
    Capture2<"then", NodeStatement>,
    Capture2<"else", Opt<NodeStatementElse>>
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementReturn : public CNode, public PatternWrapper<NodeStatementReturn> {
  using pattern =
  Seq<
    Keyword<"return">,
    Capture2<"value", Opt<NodeExpression>>,
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementCase : public CNode, public PatternWrapper<NodeStatementCase> {
  // clang-format off
  using pattern =
  Seq<
    Keyword<"case">,
    Capture2<"condition", NodeExpression>,
    // case 1...2: - this is supported by GCC?
    Opt<Seq<NodeEllipsis, NodeExpression>>,
    Atom<':'>,
    Any<
      Seq<
        Not<Keyword<"case">>,
        Not<Keyword<"default">>,
        Capture2<"body", NodeStatement>
      >
    >
  >;
  // clang-format on
};

struct NodeStatementDefault : public CNode, public PatternWrapper<NodeStatementDefault> {
  // clang-format off
  using pattern =
  Seq<
    Keyword<"default">,
    Atom<':'>,
    Any<
      Seq<
        Not<Keyword<"case">>,
        Not<Keyword<"default">>,
        Capture2<"body", NodeStatement>
      >
    >
  >;
  // clang-format on
};

struct NodeStatementSwitch : public CNode, public PatternWrapper<NodeStatementSwitch> {
  // clang-format off
  using pattern =
  Seq<
    Keyword<"switch">,
    Capture2<"condition", NodeExpression>,
    Atom<'{'>,
    Any<
      Capture2<"case", NodeStatementCase>,
      Capture2<"default", NodeStatementDefault>
    >,
    Atom<'}'>
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

struct NodeStatementWhile : public CNode,
                            public PatternWrapper<NodeStatementWhile> {
  // clang-format on
  using pattern =
  Seq<
    Keyword<"while">,
    DelimitedList<
      Atom<'('>,
      Capture2<"condition", NodeExpression>,
      Atom<','>,
      Atom<')'>
    >,
    Capture2<"body", NodeStatement>
  >;
  // clang-format off
};

//------------------------------------------------------------------------------

struct NodeStatementDoWhile : public CNode,
                              public PatternWrapper<NodeStatementDoWhile> {
  // clang-format off
  using pattern =
  Seq<
    Keyword<"do">,
    Capture2<"body", NodeStatement>,
    Keyword<"while">,
    DelimitedList<
      Atom<'('>,
      Capture2<"condition", NodeExpression>,
      Atom<','>,
      Atom<')'>
    >,
    Atom<';'>
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

struct NodeStatementLabel : public CNode, public PatternWrapper<NodeStatementLabel> {
  using pattern =
  Seq<
    Capture2<"name", NodeIdentifier>,
    Atom<':'>,
    Opt<Atom<';'>>
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementBreak : public CNode, public PatternWrapper<NodeStatementBreak> {
  using pattern = Seq<Keyword<"break">, Atom<';'>>;
};

struct NodeStatementContinue : public CNode, public PatternWrapper<NodeStatementContinue> {
  using pattern = Seq<Keyword<"continue">, Atom<';'>>;
};

//------------------------------------------------------------------------------

struct NodeAsmRef : public CNode, public PatternWrapper<NodeAsmRef> {
  // clang-format off
  using pattern =
  Seq<
    Capture2<"name", NodeAtom<LEX_STRING>>,
    Opt<Seq<
      Atom<'('>,
      Capture2<"value", NodeExpression>,
      Atom<')'>
    >>
  >;
  // clang-format on
};

struct NodeAsmRefs : public CNode, public PatternWrapper<NodeAsmRefs> {
  using pattern =
  comma_separated<
    Capture2<"ref", NodeAsmRef>
  >;
};

//------------------------------------------------------------------------------

struct NodeAsmQualifiers : public CNode, public PatternWrapper<NodeAsmQualifiers> {
  // clang-format off
  using pattern =
  Some<
    NodeKeyword<"volatile">,
    NodeKeyword<"__volatile">,
    NodeKeyword<"__volatile__">,
    NodeKeyword<"inline">,
    NodeKeyword<"goto">
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

struct NodeStatementAsm : public CNode, public PatternWrapper<NodeStatementAsm> {
  // clang-format off
  using pattern =
  Seq<
    Oneof<
      Keyword<"asm">,
      Keyword<"__asm">,
      Keyword<"__asm__">
    >,
    Opt<Capture2<"qualifiers", NodeAsmQualifiers>>,
    Atom<'('>,
    NodeAtom<LEX_STRING>,  // assembly code
    SeqOpt<
      // output operands
      Seq<Atom<':'>, Opt<Capture2<"output", NodeAsmRefs>>>,
      // input operands
      Seq<Atom<':'>, Opt<Capture2<"input", NodeAsmRefs>>>,
      // clobbers
      Seq<Atom<':'>, Opt<Capture2<"clobbers", NodeAsmRefs>>>,
      // GotoLabels
      Seq<Atom<':'>, Opt<Capture2<"gotos", comma_separated<NodeIdentifier>>>>
    >,
    Atom<')'>,
    Atom<';'>
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

struct NodeTypedef : public CNode, public PatternWrapper<NodeTypedef> {
  // clang-format off
  using pattern =
  Seq<
    Opt<
      Keyword<"__extension__">
    >,
    Keyword<"typedef">,
    Capture2<
      "newtype",
      Oneof<
        NodeStruct,
        NodeUnion,
        NodeClass,
        NodeEnum,
        NodeDeclaration
      >
    >
  >;
  // clang-format on

  static void extract_declarator(CContext& ctx, NodeDeclarator* decl) {
    if (auto id = decl->child("identifier")) {
      ctx.add_typedef_type(id->span.a);
    }

    //for (auto child : decl) {
    for (auto child = decl->child_head(); child; child = child->node_next()) {
      if (auto decl = child->as_a<NodeDeclarator>()) {
        extract_declarator(ctx, decl);
      }
    }
  }

  static void extract_declarator_list(CContext& ctx, NodeDeclaratorList* decls) {
    if (!decls) return;
    for (auto child = decls->child_head(); child; child = child->node_next()) {
      if (auto decl = child->as_a<NodeDeclarator>()) {
        extract_declarator(ctx, decl);
      }
    }
  }

  static void extract_type(CContext& ctx) {
    auto node = ctx.top_tail();

    // node->dump_tree();

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

    matcheroni_assert(false);
  }

  static lex_span match(CContext& ctx, lex_span s) {
    auto end = pattern::match(ctx, s);
    if (end) extract_type(ctx);
    return end;
  }
};

//------------------------------------------------------------------------------

struct NodeStatementGoto : public CNode, public PatternWrapper<NodeStatementGoto> {
  // pr21356.c - Spec says goto should be an identifier, GCC allows expressions
  using pattern = Seq<Keyword<"goto">, NodeExpression, Atom<';'>>;
};

//------------------------------------------------------------------------------

struct NodeStatement : public PatternWrapper<NodeStatement> {
  // clang-format off
  using pattern =
  Oneof<
    // All of these have keywords or something first
    Capture2<"class",    Seq<NodeClass,   Atom<';'>>>,
    Capture2<"struct",   Seq<NodeStruct,  Atom<';'>>>,
    Capture2<"union",    Seq<NodeUnion,   Atom<';'>>>,
    Capture2<"enum",     Seq<NodeEnum,    Atom<';'>>>,
    Capture2<"typedef",  Seq<NodeTypedef, Atom<';'>>>,

    Capture2<"for",      NodeStatementFor>,
    Capture2<"if",       NodeStatementIf>,
    Capture2<"return",   NodeStatementReturn>,
    Capture2<"switch",   NodeStatementSwitch>,
    Capture2<"dowhile",  NodeStatementDoWhile>,
    Capture2<"while",    NodeStatementWhile>,
    Capture2<"goto",     NodeStatementGoto>,
    Capture2<"asm",      NodeStatementAsm>,
    Capture2<"compound", NodeStatementCompound>,
    Capture2<"break",    NodeStatementBreak>,
    Capture2<"continue", NodeStatementContinue>,

    // These don't - but they might confuse a keyword with an identifier...
    Capture2<"label",    NodeStatementLabel>,
    Capture2<"function", NodeFunctionDefinition>,

    // If declaration is before expression, we parse "x = 1;" as a declaration
    // because it matches a declarator (bare identifier) + initializer list :/
    Capture2<"expression",  Seq<comma_separated<NodeExpression>, Atom<';'>>>,
    Capture2<"declaration", Seq<NodeDeclaration, Atom<';'>>>,

    // Extra semicolons
    Atom<';'>
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

struct NodeTranslationUnit : public CNode, public PatternWrapper<NodeTranslationUnit> {
  // clang-format off
  using pattern =
  Any<
    Oneof<
      Capture2<"class",       Seq<NodeClass,  Atom<';'>>>,
      Capture2<"struct",      Seq<NodeStruct, Atom<';'>>>,
      Capture2<"union",       Seq<NodeUnion,  Atom<';'>>>,
      Capture2<"enum",        Seq<NodeEnum,   Atom<';'>>>,
      Capture2<"typedef",     NodeTypedef>,
      Capture2<"preproc",     NodePreproc>,
      Capture2<"template",    Seq<NodeTemplate, Atom<';'>>>,
      Capture2<"function",    NodeFunctionDefinition>,
      Capture2<"declaration", Seq<NodeDeclaration, Atom<';'>>>,
      Atom<';'>
    >
  >;
  // clang-format on
};

//------------------------------------------------------------------------------
