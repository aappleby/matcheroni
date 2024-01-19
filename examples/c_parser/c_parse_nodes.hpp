// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once

#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"
#include "matcheroni/Utilities.hpp"

#include "examples/c_parser/c_constants.hpp"
#include "examples/c_lexer/CToken.hpp"
#include "examples/c_parser/CNode.hpp"
#include "examples/c_parser/CContext.hpp"
#include "examples/SST.hpp"

using namespace matcheroni;
using namespace parseroni;

#if 0
template <StringParam match_name, typename P>
struct TraceToken {

  template<typename node_type>
  inline static void print_match2(TokenSpan span, TokenSpan tail, int width) {

    if (tail.is_valid()) {
      const char* tail_a = tail.begin->text_head();
      print_match(text_a, tail_a, text_b, 0x80FF80, 0xCCCCCC, width);
    } else {
      const char* tail_b = tail.end->text_head();
      print_match(text_a, tail_b, text_b, 0xCCCCCC, 0x8080FF, width);
    }
  }

  static TokenSpan match(CContext& ctx, TokenSpan body) {
    matcheroni_assert(body.is_valid());
    if (body.is_empty()) return body.fail();

    auto text = TextSpan(body.begin->begin, (body.end - 1)->end);
    auto name = match_name.str_val;

    int depth = ctx.trace_depth++;

    print_match2(text, text, 40);
    print_trellis(depth, name, "?", 0xCCCCCC);
    printf("\n");

    //print_bar(ctx.trace_depth++, text, name, "?");
    auto tail = P::match(ctx, body);
    depth = --ctx.trace_depth;

    print_match2(body, tail, 40);

    if (tail.is_valid()) {
      print_trellis(depth, name, "!", 0x80FF80);
      printf("\n");
      utils::print_context(ctx.text_span, ctx, 40);
      printf("\n");
    }
    else {
      print_trellis(depth, name, "X", 0x8080FF);
      printf("\n");
    }


    //print_bar(--ctx.trace_depth, text, name, tail.is_valid() ? "OK" : "X");

    return tail;
  }
};
#endif

//template <StringParam match_name, typename pattern>
//using Cap = parseroni::Capture<match_name, pattern, CNode>;

//template <StringParam match_name, typename pattern>
//using Cap = TraceToken<match_name, Capture<match_name, pattern, CNode>>;

//------------------------------------------------------------------------------

struct NodeAbstractDeclarator;
struct NodeClass;
struct NodeConstructor;
struct NodeDeclaration;
struct NodeDeclarator;
struct NodeEnum;
struct NodeExpression;
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

  static TokenSpan match(CContext& ctx, TokenSpan body) {
    if (!body.is_valid() || body.is_empty()) return body.fail();
    if (ctx.atom_cmp(*body.begin, LEX_KEYWORD) != 0) return body.fail();
    if (ctx.atom_cmp(*body.begin, lit.span()) != 0) return body.fail();
    return body.advance(1);
  }
};

template <StringParam lit>
struct Literal2 : public CNode, PatternWrapper<Literal2<lit>> {
  static TokenSpan match(CContext& ctx, TokenSpan body) {
    if (!body.is_valid() || body.is_empty()) return body.fail();
    if (ctx.atom_cmp(*body.begin, lit.span()) != 0) return body.fail();
    return body.advance(1);
  }
};

//------------------------------------------------------------------------------

template <StringParam lit>
inline TokenSpan match_punct(CContext& ctx, TokenSpan body) {
  if (!body.is_valid() || body.is_empty()) return body.fail();

  size_t lit_len = lit.str_len;
  const char* lit_val = lit.str_val;

  if (body.len() < lit.str_len) return body.fail();

  for (auto i = 0; i < lit.str_len; i++) {
    const CToken& tok_a = body.begin[0];
    if (ctx.atom_cmp(tok_a, LEX_PUNCT) != 0) return body.fail();
    if (ctx.atom_cmp(tok_a.text.begin[0], lit.str_val[i]) != 0) return body.fail();
    body = body.advance(1);
  }

  return body;
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
  using match_base   = Ref<&CContext::match_builtin_type_base>;
  using match_suffix = Ref<&CContext::match_builtin_type_suffix>;

  // clang-format off
  using pattern =
  Seq<
    Any<
      Seq<
        Capture<"prefix", match_prefix, CNode>,
        And<match_base>
      >
    >,
    Capture<"base_type", match_base, CNode>,
    Opt<Capture<"suffix", match_suffix, CNode>>
  >;
  // clang-format on
};

struct NodeTypedefType : public CNode, PatternWrapper<NodeTypedefType> {
  using pattern = Capture<"typedef", Ref<&CContext::match_typedef_type>, CNode>;
};

//------------------------------------------------------------------------------
// Excluding builtins and typedefs from being identifiers changes the total
// number of parse nodes, but why?

// - Because "uint8_t *x = 5" gets misparsed as an expression if uint8_t matches
// as an identifier

using lit_identifier =
Seq<
  Not<NodeBuiltinType>,
  Not<NodeTypedefType>,
  Atom<LEX_IDENTIFIER>
>;

struct NodeIdentifier : public CNode {};

//------------------------------------------------------------------------------

struct NodePreproc : public CNode {
  using pattern = Atom<LEX_PREPROC>;

  static TokenSpan match(CContext& ctx, TokenSpan body) {
    auto tail = pattern::match(ctx, body);
    if (tail.is_valid()) {
      std::string s(body.begin->text.begin, (tail.begin - 1)->text.end);

      if (s.find("stdio") != std::string::npos) {
        for (auto t : stdio_typedefs) {
          ctx.type_scope->add_typedef_type(t);
        }
      }

      if (s.find("stdint") != std::string::npos) {
        for (auto t : stdint_typedefs) {
          ctx.type_scope->add_typedef_type(t);
        }
      }

      if (s.find("stddef") != std::string::npos) {
        for (auto t : stddef_typedefs) {
          ctx.type_scope->add_typedef_type(t);
        }
      }

    }
    return tail;
  }
};

//------------------------------------------------------------------------------

struct NodeConstant : public CNode {};

using exp_constant =
Oneof<
  Atom<LEX_FLOAT>,
  Atom<LEX_INT>,
  Atom<LEX_CHAR>,
  Some<Atom<LEX_STRING>>
>;


//------------------------------------------------------------------------------

template <StringParam lit>
struct NodePrefixOp : public CNode {
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
  static TokenSpan match(CContext& ctx, TokenSpan body) {
    matcheroni_assert(body.is_valid());
    TextSpan span = body.begin->text;
    if (SST<qualifiers>::match(span.begin, span.end)) {
      return body.advance(1);
    }
    else {
      return body.fail();
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
    Capture<"code", Some<NodeAtom<LEX_STRING>>, CNode>,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

using lit_access_specifier =
Seq<
  Oneof<
    Literal2<"public">,
    Literal2<"private">
  >,
  Atom<':'>
>;

struct NodeAccessSpecifier : public CNode {};

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
  DelimitedList<
    Atom<'('>,
    Capture<"expression", NodeExpression, CNode>,
    Atom<','>,
    Atom<')'>
  >;
};

struct NodeSuffixParen : public CNode, PatternWrapper<NodeSuffixParen> {
  NodeSuffixParen() {
    precedence = 2;
    assoc = 2;
  }

  using pattern =
  DelimitedList<
    Atom<'('>,
    Capture<"expression", NodeExpression, CNode>,
    Atom<','>,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeExpressionBraces : public CNode, PatternWrapper<NodeExpressionBraces> {
  using pattern =
  DelimitedList<
    Atom<'{'>,
    Capture<"expression", NodeExpression, CNode>,
    Atom<','>,
    Atom<'}'>
  >;
};

using suffix_braces =
DelimitedList<
  Atom<'{'>,
  Capture<"expression", NodeExpression, CNode>,
  Atom<','>,
  Atom<'}'>
>;

struct NodeSuffixBraces : public CNode {
  NodeSuffixBraces() {
    precedence = 2;
    assoc = 2;
  }
};


//------------------------------------------------------------------------------

struct NodeSuffixSubscript : public CNode, PatternWrapper<NodeSuffixSubscript> {
  NodeSuffixSubscript() {
    precedence = 2;
    assoc = 2;
  }

  using pattern =
  DelimitedList<
    Atom<'['>,
    Capture<"expression", NodeExpression, CNode>,
    Atom<','>,
    Atom<']'>
  >;
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
    Capture<
      "value",
      Oneof<
        Capture<"type",       NodeParenType, CNode>,
        Capture<"expression", NodeExpressionParen, CNode>,
        Capture<"expression", NodeExpression, CNode>
      >,
      CNode
    >
  >;
};

struct NodeExpressionAlignof : public CNode, PatternWrapper<NodeExpressionAlignof> {
  using pattern =
  Seq<
    Keyword<"__alignof__">,
    Capture<
      "value",
      Oneof<
        Capture<"type",       NodeParenType, CNode>,
        Capture<"expression", NodeExpressionParen, CNode>
      >,
      CNode
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
    Capture<"name", NodeTypeName, CNode>,
    Atom<','>,
    Capture<"expression", NodeExpression, CNode>,
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
  Capture<"cast",      NodePrefixCast,              CNode>,
  Capture<"extension", Keyword<"__extension__">,    NodePrefixKeyword<"__extension__">>,
  Capture<"real",      Keyword<"__real">,           NodePrefixKeyword<"__real">>,
  Capture<"real",      Keyword<"__real__">,         NodePrefixKeyword<"__real__">>,
  Capture<"imag",      Keyword<"__imag">,           NodePrefixKeyword<"__imag">>,
  Capture<"imag",      Keyword<"__imag__">,         NodePrefixKeyword<"__imag__">>,
  Capture<"preinc",    NodePrefixOp<"++">::pattern, NodePrefixOp<"++">>,
  Capture<"predec",    NodePrefixOp<"--">::pattern, NodePrefixOp<"--">>,
  Capture<"preplus",   NodePrefixOp<"+">::pattern,  NodePrefixOp<"+">>,
  Capture<"preminus",  NodePrefixOp<"-">::pattern,  NodePrefixOp<"-">>,
  Capture<"prebang",   NodePrefixOp<"!">::pattern,  NodePrefixOp<"!">>,
  Capture<"pretilde",  NodePrefixOp<"~">::pattern,  NodePrefixOp<"~">>,
  Capture<"prestar",   NodePrefixOp<"*">::pattern,  NodePrefixOp<"*">>,
  Capture<"preamp",    NodePrefixOp<"&">::pattern,  NodePrefixOp<"&">>
>;
// clang-format on

//----------------------------------------

// clang-format off
using ExpressionCore =
Oneof<
  Capture<"sizeof",       NodeExpressionSizeof,      CNode>,
  Capture<"alignof",      NodeExpressionAlignof,     CNode>,
  Capture<"offsetof",     NodeExpressionOffsetof,    CNode>,
  Capture<"gcc_compound", NodeExpressionGccCompound, CNode>,
  Capture<"paren",        NodeExpressionParen,       CNode>,
  Capture<"init",         NodeInitializerList,       CNode>,
  Capture<"braces",       NodeExpressionBraces,      CNode>,
  Capture<"identifier",   lit_identifier,            CNode>,
  Capture<"constant",     exp_constant,              NodeConstant>
>;
// clang-format on

//----------------------------------------

// clang-format off
using ExpressionSuffixOp =
Oneof<
  Capture<"initializer", NodeSuffixInitializerList, CNode>,  // must be before NodeSuffixBraces
  Capture<"braces",      suffix_braces,             NodeSuffixBraces>,
  Capture<"paren",       NodeSuffixParen,           CNode>,
  Capture<"subscript",   NodeSuffixSubscript,       CNode>,
  Capture<"postinc",     NodeSuffixOp<"++">,        CNode>,
  Capture<"postdec",     NodeSuffixOp<"--">,        CNode>
>;
// clang-format on

//----------------------------------------

// clang-format off
using unit_pattern =
Seq<
  Any<
    Capture<"prefix", ExpressionPrefixOp, CNode>
  >,
  ExpressionCore,
  Any<
    Capture<"suffix", ExpressionSuffixOp, CNode>
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
      Capture<"then", comma_separated<NodeExpression>, CNode>
    >,
    NodeBinaryOp<":">
  >;
};

//----------------------------------------

struct NodeExpression : public CNode, PatternWrapper<NodeExpression> {
  static TokenSpan match_binary_op(CContext& ctx, TokenSpan body) {
    matcheroni_assert(body.is_valid());

    if (ctx.atom_cmp(*body.begin, LEX_PUNCT)) {
      return body.fail();
    }

    // clang-format off
    switch (body.begin->text.begin[0]) {
      case '+':
        return Oneof<NodeBinaryOp<"+=">, NodeBinaryOp<"+">>::match(ctx, body);
      case '-':
        return Oneof<NodeBinaryOp<"->*">, NodeBinaryOp<"->">, NodeBinaryOp<"-=">, NodeBinaryOp<"-">>::match(ctx, body);
      case '*':
        return Oneof<NodeBinaryOp<"*=">, NodeBinaryOp<"*">>::match(ctx, body);
      case '/':
        return Oneof<NodeBinaryOp<"/=">, NodeBinaryOp<"/">>::match(ctx, body);
      case '=':
        return Oneof<NodeBinaryOp<"==">, NodeBinaryOp<"=">>::match(ctx, body);
      case '<':
        return Oneof<NodeBinaryOp<"<<=">, NodeBinaryOp<"<=>">, NodeBinaryOp<"<=">, NodeBinaryOp<"<<">, NodeBinaryOp<"<">>::match(ctx, body);
      case '>':
        return Oneof<NodeBinaryOp<">>=">, NodeBinaryOp<">=">, NodeBinaryOp<">>">, NodeBinaryOp<">">>::match(ctx, body);
      case '!':
        return NodeBinaryOp<"!=">::match(ctx, body);
      case '&':
        return Oneof<NodeBinaryOp<"&&">, NodeBinaryOp<"&=">, NodeBinaryOp<"&">>::match(ctx, body);
      case '|':
        return Oneof<NodeBinaryOp<"||">, NodeBinaryOp<"|=">, NodeBinaryOp<"|">>::match(ctx, body);
      case '^':
        return Oneof<NodeBinaryOp<"^=">, NodeBinaryOp<"^">>::match(ctx, body);
      case '%':
        return Oneof<NodeBinaryOp<"%=">, NodeBinaryOp<"%">>::match(ctx, body);
      case '.':
        return Oneof<NodeBinaryOp<".*">, NodeBinaryOp<".">>::match(ctx, body);
      case '?':
        return NodeTernaryOp::match(ctx, body);

        // FIXME this is only for C++, and
        // case ':': return NodeBinaryOp<"::">::match(ctx, body);
        // default:  return nullptr;
    }
    // clang-format on

    return body.fail();
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

  static TokenSpan match2(CContext& ctx, TokenSpan body) {

    using pattern =
    Seq<
      Capture<"unit", unit_pattern, CNode>,
      Any<
        Seq<
          Capture<"op", Ref<match_binary_op>, CNode>,
          Capture<"unit", unit_pattern, CNode>
        >
      >
    >;

    auto tail = pattern::match(ctx, body);
    if (!tail.is_valid()) {
      return body.fail();
    }

    //auto tok_a = body.begin;
    //auto tok_b = tail.begin;

    /*
    while (0) {
      {
        auto c = tok_a;
        while (c && c < tok_b) {
          c = c->step_right();
        }
        printf("\n");
      }

      auto c = tok_a;
      while (c && c->span->precedence && c < tok_b) {
        c = c->step_right();
      }

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
    if (auto tail = SpanTernaryOp::match(ctx, cursor, b)) {
      auto node = new NodeExpressionTernary();
      node->init(a, tail - 1);
      cursor = tail;
    }
#endif

#if 0
    {
      const CToken* c = a;
      while(1) {
        if (auto tail = c->span->tok_b()) {
          c = tail + 1;
        }
        else {
          c++;
        }
        if (c == tok_b) break;
      }
    }
#endif

    return tail;
  }

  //----------------------------------------

  static TokenSpan match(CContext& ctx, TokenSpan body) {
    //print_trace_start<NodeExpression, CToken>(a);
    auto tail = match2(ctx, body);
    if (tail.is_valid()) {
      //auto node = new NodeExpression();
      //node->init_node(ctx, a, tail - 1, a->span, (tail - 1)->span);
    }
    //print_trace_end<NodeExpression, CToken>(a, tail);
    return tail;
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
        Capture<"expression", NodeExpression, CNode>,
        Capture<"keyword", Atom<LEX_KEYWORD>, CNode>
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
    Capture<
      "value",
      Oneof<
        NodeTypeDecl,
        exp_constant
      >,
      CNode
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
    Capture<"identifier", lit_identifier, CNode>,
    Atom<')'>
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

struct NodeModifier : public PatternWrapper<NodeModifier> {
  // This is the slowest matcher in the app, why?
  using pattern =
  Oneof<
    Capture<"alignas",   NodeAlignas,   CNode>,
    Capture<"declspec",  NodeDeclspec,  CNode>,
    Capture<"attribute", NodeAttribute, CNode>,
    Capture<"qualifier", NodeQualifier, CNode>
  >;
};

//------------------------------------------------------------------------------

struct NodeTypeDecl : public CNode, public PatternWrapper<NodeTypeDecl> {
  using pattern =
  Seq<
    Any<
      Capture<"modifier", NodeModifier, CNode>
    >,
    Capture<"specifier", NodeSpecifier, CNode>,
    Opt<
      Capture<"abstract_decl", NodeAbstractDeclarator, CNode>
    >
  >;
};

//------------------------------------------------------------------------------

struct NodePointer : public CNode, public PatternWrapper<NodePointer> {
  using pattern =
  Seq<
    Capture<"star", Literal2<"*">, CNode>,
    Any<
      Capture<"star", Literal2<"*">, CNode>,
      Capture<"modifier", NodeModifier, CNode>
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeEllipsis : public CNode, public PatternWrapper<NodeEllipsis> {
  using pattern = Ref<match_punct<"...">>;
};


//------------------------------------------------------------------------------

using parameter =
Oneof<
  Capture<"ellipsis", NodeEllipsis, CNode>,
  Seq<
    Any<Capture<"modifier", NodeModifier, CNode>>,
    Capture<"specifier", NodeSpecifier, CNode>,
    Any<Capture<"modifier", NodeModifier, CNode>>,
    Capture<"decl", Opt<NodeDeclarator, NodeAbstractDeclarator>, CNode>
  >,
  Capture<"identifier", lit_identifier, CNode>
>;

struct NodeParam : public CNode {};

//------------------------------------------------------------------------------

using param_list = DelimitedList<
  Atom<'('>,
  Capture<"param", parameter, NodeParam>,
  Atom<','>,
  Atom<')'>
>;

struct NodeParamList : public CNode {};

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
  DelimitedList<
    Atom<'<'>,
    Capture<"arg", NodeExpression, CNode>,
    Atom<','>,
    Atom<'>'>
  >;
};

//------------------------------------------------------------------------------

struct NodeAtomicType : public CNode, public PatternWrapper<NodeAtomicType> {
  using pattern =
  Seq<
    Keyword<"_Atomic">,
    Atom<'('>,
    Capture<"decl", NodeTypeDecl, CNode>,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeSpecifier : public CNode, public PatternWrapper<NodeSpecifier> {
  // clang-format off
  using pattern =
  Seq<
    Oneof<
      // These have to be NodeIdentifier because "void foo(struct S);"
      // is valid even without the definition of S.
      Seq<NodeLiteral<"class">, Oneof<lit_identifier, NodeTypedefType>>,
      Seq<Keyword<"union">,     Oneof<lit_identifier, NodeTypedefType>>,
      Seq<Keyword<"struct">,    Oneof<lit_identifier, NodeTypedefType>>,
      Seq<Keyword<"enum">,      Oneof<lit_identifier, NodeTypedefType>>,

      /*
      // If this was C++, we would also need to match these directly
      BaseClassType,
      BaseStructType,
      NodeUnionType,
      NodeEnumType,
      */

      Capture<"builtin", NodeBuiltinType, CNode>,
      Capture<"typedef", NodeTypedefType, CNode>,
      Capture<"atomic",  NodeAtomicType,  CNode>,
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
  using pattern = Seq<Atom<':'>, Capture<"expression", NodeExpression, CNode>>;
};

//------------------------------------------------------------------------------
// FIXME this can match nothing and that seems wrong

struct NodeAbstractDeclarator : public CNode,
                                public PatternWrapper<NodeAbstractDeclarator> {
  using pattern =
  Seq<
    Opt<Capture<"pointer", NodePointer, CNode>>,
    Opt<
      Seq<
        Atom<'('>,
        Capture<"abstract_decl", NodeAbstractDeclarator, CNode>,
        Atom<')'>
      >
    >,
    Any<
      Capture<"attribute",    NodeAttribute, CNode>,
      Capture<"array_suffix", NodeArraySuffix, CNode>,
      Capture<"param_list", param_list, NodeParamList>
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeDeclarator : public CNode, public PatternWrapper<NodeDeclarator> {
  using pattern =
  Seq<
    Any<
      Capture<"attribute", NodeAttribute, CNode>,
      Capture<"modifier",  NodeModifier,  CNode>,
      Capture<"pointer",   NodePointer,   CNode>
    >,
    Oneof<
      Capture<"identifier", lit_identifier, CNode>,
      Capture<"declarator",
        Seq<
          Atom<'('>,
          NodeDeclarator,
          Atom<')'>
        >
      , CNode>
    >,
    Any<
      Capture<"asm_suffix",   NodeAsmSuffix,   CNode>,
      Capture<"bit_suffix",   NodeBitSuffix,   CNode>,
      Capture<"attribute",    NodeAttribute,   CNode>,
      Capture<"array_suffix", NodeArraySuffix, CNode>,
      Capture<"param_list",   param_list,      NodeParamList>
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeDeclaratorList : public CNode,
                            public PatternWrapper<NodeDeclaratorList> {
  // clang-format off
  using pattern =
  comma_separated<
    Capture<
      "decl",
      Seq<
        Oneof<
          Seq<
            Capture<"name",           NodeDeclarator, CNode>,
            Opt<Capture<"bit_suffix", NodeBitSuffix,  CNode>>
          >,
          Capture<"bit_suffix", NodeBitSuffix, CNode>
        >,
        Opt<
          Seq<
            Atom<'='>,
            Capture<"initializer", NodeInitializer, CNode>
          >
        >
      >,
      CNode
    >
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

struct NodeFunctionIdentifier : public CNode,
                                public PatternWrapper<NodeFunctionIdentifier> {
  // clang-format off
  using pattern =
  Seq<
    Any<NodeAttribute, NodePointer>,
    Oneof<
      lit_identifier,
      Seq<Atom<'('>, NodeFunctionIdentifier, Atom<')'>>
    >
  >;
  // clang-format on
};

// function-definition:
//     declaration-specifiers declarator declaration-listopt compound-statement

using definition_function =
Seq<
  Any<Capture<"modifier",         NodeModifier,                    CNode>>,
  Opt<Capture<"func_return_type", NodeSpecifier,                   CNode>>,
  Any<Capture<"modifier",         NodeModifier,                    CNode>>,
  One<Capture<"func_identifier",  NodeFunctionIdentifier,          CNode>>,
  One<Capture<"func_params",      param_list,                      NodeParamList>>,
  Any<Capture<"modifier",         NodeModifier,                    CNode>>,
  Opt<Capture<"asm_suffix",       NodeAsmSuffix,                   CNode>>,
  Opt<Capture<"const",            NodeKeyword<"const">,            CNode>>,
  Any<Capture<"old_declaration",  Seq<NodeDeclaration, Atom<';'>>, CNode>>, // This is old-style declarations after param list
  One<Capture<"func_body",        NodeStatementCompound,           CNode>>
>;

struct NodeFunctionDefinition : public CNode {};

//------------------------------------------------------------------------------

struct NodeField : public PatternWrapper<NodeField> {
  // clang-format off
  using pattern =
  Oneof<
    Atom<';'>,
    Capture<"access",      lit_access_specifier, NodeAccessSpecifier>,
    Capture<"constructor", NodeConstructor,      NodeConstructor>,
    Capture<"function",    definition_function,  NodeFunctionDefinition>,
    Capture<"struct",      NodeStruct,           NodeStruct>,
    Capture<"union",       NodeUnion,            NodeUnion>,
    Capture<"template",    NodeTemplate,         NodeTemplate>,
    Capture<"class",       NodeClass,            NodeClass>,
    Capture<"enum",        NodeEnum,             NodeEnum>,
    Capture<"decl",        NodeDeclaration,      NodeDeclaration>
  >;
  // clang-format on
};

struct NodeFieldList : public CNode, public PatternWrapper<NodeFieldList> {
  using pattern =
  DelimitedBlock<
    Atom<'{'>,
    NodeField,
    Atom<'}'>
  >;
};

//------------------------------------------------------------------------------

struct NodeNamespace : public CNode, public PatternWrapper<NodeNamespace> {
  using pattern =
  Seq<
    Keyword<"namespace">,
    Capture<"name", Opt<lit_identifier>, CNode>,
    Opt<NodeFieldList>
  >;
};

//------------------------------------------------------------------------------

struct NodeStructType : public CNode, public PatternWrapper<NodeStructType> {
  using pattern = Ref<&CContext::match_struct_type>;
};

struct NodeStructTypeAdder : public NodeIdentifier {
  static TokenSpan match(CContext& ctx, TokenSpan body) {
    auto tail = lit_identifier::match(ctx, body);
    if (tail.is_valid()) {
      ctx.add_struct_type(body.begin);
      return tail;
    } else {
      auto tail = NodeTypedefType::match(ctx, body);
      if (tail.is_valid()) {
        // Already typedef'd
        return tail;
      } else {
        return body.fail();
      }
    }
  }
};

struct NodeStruct : public CNode, public PatternWrapper<NodeStruct> {
  // clang-format off
  using pattern =
  Seq<
    Any<Capture<"modifier",  NodeModifier, CNode>>,
    Keyword<"struct">,
    Any<Capture<"attribute", NodeAttribute, CNode>>,  // This has to be here, there are a lot of struct __attrib__() foo {};
    Opt<NodeStructTypeAdder>,
    Opt<Capture<"fields",    NodeFieldList, CNode>>,
    Any<Capture<"attribute", NodeAttribute, CNode>>,
    Opt<Capture<"decls",     NodeDeclaratorList, CNode>>
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

struct NodeUnionType : public CNode {
  static TokenSpan match(CContext& ctx, TokenSpan body) {
    return ctx.match_union_type(body);
  }
};

struct NodeUnionTypeAdder : public NodeIdentifier {
  static TokenSpan match(CContext& ctx, TokenSpan body) {
    auto tail = lit_identifier::match(ctx, body);
    if (tail.is_valid()) {
      ctx.add_union_type(body.begin);
      return tail;
    } else {
      auto tail = NodeTypedefType::match(ctx, body);
      if (tail.is_valid()) {
        // Already typedef'd
        return tail;
      } else {
        return body.fail();
      }
    }
  }
};

struct NodeUnion : public CNode, public PatternWrapper<NodeUnion> {
  // clang-format off
  using pattern =
  Seq<
    Any<Capture<"modifier", NodeModifier, CNode>>,
    Keyword<"union">,
    Any<Capture<"attribute", NodeAttribute, CNode>>,
    Opt<NodeUnionTypeAdder>,
    Opt<Capture<"fields", NodeFieldList, CNode>>,
    Any<Capture<"attribute", NodeAttribute, CNode>>,
    Opt<Capture<"decls", NodeDeclaratorList, CNode>>
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

struct NodeClassType : public CNode, public PatternWrapper<NodeClassType> {
  using pattern = Ref<&CContext::match_class_type>;
};

struct NodeClassTypeAdder : public NodeIdentifier {
  static TokenSpan match(CContext& ctx, TokenSpan body) {
    auto tail = lit_identifier::match(ctx, body);
    if (tail.is_valid()) {
      ctx.add_class_type(body.begin);
      return tail;
    } else
    {
      auto tail = NodeTypedefType::match(ctx, body);
      if (tail.is_valid()) {
        // Already typedef'd
        return tail;
      } else {
        return body.fail();
      }
    }
  }
};

struct NodeClass : public CNode, public PatternWrapper<NodeClass> {
  // clang-format off
  using pattern =
  Seq<
    Any<Capture<"modifier",  NodeModifier, CNode>>,
    NodeLiteral<"class">,
    Any<Capture<"attribute", NodeAttribute, CNode>>,
    Opt<NodeClassTypeAdder>,
    Opt<Capture<"body",      NodeFieldList, CNode>>,
    Any<Capture<"attribute", NodeAttribute, CNode>>,
    Opt<Capture<"decls",     NodeDeclaratorList, CNode>>
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

using template_param_list =
DelimitedList<
  Atom<'<'>,
  Capture<"param", NodeDeclaration, CNode>,
  Atom<','>,
  Atom<'>'>
>;

struct NodeTemplateParams : public CNode {};

struct NodeTemplate : public CNode, public PatternWrapper<NodeTemplate> {
  using pattern =
  Seq<
    NodeLiteral<"template">,
    Capture<"params", template_param_list, NodeTemplateParams>,
    Capture<"class", NodeClass, CNode>
  >;
};

//------------------------------------------------------------------------------
// FIXME should probably have a few diffeerent versions instead of all the opts

struct NodeEnumType : public CNode, public PatternWrapper<NodeEnumType> {
  using pattern = Ref<&CContext::match_enum_type>;
};

struct NodeEnumTypeAdder : public NodeIdentifier {
  static TokenSpan match(CContext& ctx, TokenSpan body) {
    auto tail = lit_identifier::match(ctx, body);
    if (tail.is_valid()) {
      ctx.add_enum_type(body.begin);
      return tail;
    } else {
        auto tail = NodeTypedefType::match(ctx, body);
        if (tail.is_valid()) {
        // Already typedef'd
        return tail;
      } else {
        return body.fail();
      }
    }
  }
};

struct NodeEnumerator : public CNode, public PatternWrapper<NodeEnumerator> {
  using pattern =
  Seq<
    Capture<"name", lit_identifier, CNode>,
    Opt<
      Seq<
        Atom<'='>,
        Capture<"value", NodeExpression, CNode>
      >
    >
  >;
};

struct NodeEnumerators : public CNode, public PatternWrapper<NodeEnumerators> {
  using pattern =
  DelimitedList<
    Atom<'{'>,
    Capture<"enumerator", NodeEnumerator, CNode>,
    Atom<','>,
    Atom<'}'>
  >;
};

struct NodeEnum : public CNode, public PatternWrapper<NodeEnum> {
  // clang-format off
  using pattern =
  Seq<
    Any<Capture<"modifier", NodeModifier, CNode>>,
    Keyword<"enum">,
    Opt<NodeLiteral<"class">>,
    Opt<NodeEnumTypeAdder>,
    Opt<Seq<Atom<':'>, Capture<"type", NodeTypeDecl, CNode>>>,
    Opt<Capture<"body",  NodeEnumerators, CNode>>,
    Opt<Capture<"decls", NodeDeclaratorList, CNode>>
  >;
};

//------------------------------------------------------------------------------

struct NodeDesignation : public CNode, public PatternWrapper<NodeDesignation> {
  // clang-format off
  using pattern =
  Some<
    Seq<Atom<'['>, exp_constant, Atom<']'>>,
    Seq<Atom<'['>, lit_identifier, Atom<']'>>,
    Seq<Atom<'.'>, lit_identifier>
  >;
  // clang-format on
};

struct NodeInitializerList : public CNode, public PatternWrapper<NodeInitializerList> {
  using pattern = DelimitedList<
      Atom<'{'>,
      Seq<Opt<Seq<NodeDesignation, Atom<'='>>,
              Seq<lit_identifier, Atom<':'>>  // This isn't in the C grammar but
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
        Seq<lit_identifier, Atom<':'>>  // This isn't in the C grammar but compndlit-1.c uses it?
      >,
      Capture<"initializer", NodeInitializer, CNode>
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

struct NodeConstructor : public CNode, public PatternWrapper<NodeConstructor> {
  // clang-format off
  using pattern =
  Seq<
    NodeClassType,
    Capture<"params", param_list, NodeParamList>,
    Oneof<
      Atom<';'>,
      Capture<"body", NodeStatementCompound, CNode>
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
      Capture<"attribute", NodeAttribute, CNode>,
      Capture<"modifier", NodeModifier, CNode>
    >,
    Oneof<
      Seq<
        Capture<"specifier", NodeSpecifier, CNode>,
        Any<
          Capture<"attribute", NodeAttribute, CNode>,
          Capture<"modifier", NodeModifier, CNode>
        >,
        Opt<
          Capture<"decls", NodeDeclaratorList, CNode>
        >
      >,
      Seq<
        Opt<Capture<"specifier", NodeSpecifier, CNode>>,
        Any<
          Capture<"attribute", NodeAttribute, CNode>,
          Capture<"modifier", NodeModifier, CNode>
        >,
        Capture<"decls", NodeDeclaratorList, CNode>
      >
    >
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

template <typename P>
struct PushPopScope {
  static TokenSpan match(CContext& ctx, TokenSpan body) {
    ctx.push_scope();
    auto tail = P::match(ctx, body);
    ctx.pop_scope();
    return tail;
  }
};

struct NodeStatementCompound : public CNode, public PatternWrapper<NodeStatementCompound> {
  using pattern =
  PushPopScope<
    DelimitedBlock<
      Atom<'{'>,
      Capture<"statement", NodeStatement, CNode>,
      Atom<'}'>
    >
  >;
};

//------------------------------------------------------------------------------

using statement_for =
Seq<
  Keyword<"for">,
  Atom<'('>,
  Capture<
    "init",
    // This is _not_ the same as
    // Opt<Oneof<e, x>>, Atom<';'>
    Oneof<
      Seq<comma_separated<NodeExpression>,  Atom<';'>>,
      Seq<comma_separated<NodeDeclaration>, Atom<';'>>,
      Atom<';'>
    >,
    CNode
  >,
  Capture<"condition", Opt<comma_separated<NodeExpression>>, CNode>,
  Atom<';'>,
  Capture<"step", Opt<comma_separated<NodeExpression>>, CNode>,
  Atom<')'>,
  Oneof<NodeStatementCompound, NodeStatement>
>;

struct NodeStatementFor : public CNode {};

//------------------------------------------------------------------------------

struct NodeStatementElse : public CNode, public PatternWrapper<NodeStatementElse> {
  using pattern = Seq<Keyword<"else">, NodeStatement>;
};

struct NodeStatementIf : public CNode, public PatternWrapper<NodeStatementIf> {
  using pattern =
  Seq<
    Keyword<"if">,
    Capture<
      "condition",
      DelimitedList<Atom<'('>, NodeExpression, Atom<','>, Atom<')'>>
    , CNode>,
    Capture<"then", NodeStatement, CNode>,
    Capture<"else", Opt<NodeStatementElse>, CNode>
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementReturn : public CNode, public PatternWrapper<NodeStatementReturn> {
  using pattern =
  Seq<
    Keyword<"return">,
    Capture<"value", Opt<NodeExpression>, CNode>,
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementCase : public CNode, public PatternWrapper<NodeStatementCase> {
  // clang-format off
  using pattern =
  Seq<
    Keyword<"case">,
    Capture<"condition", NodeExpression, CNode>,
    // case 1...2: - this is supported by GCC?
    Opt<Seq<NodeEllipsis, NodeExpression>>,
    Atom<':'>,
    Any<
      Seq<
        Not<Keyword<"case">>,
        Not<Keyword<"default">>,
        Capture<"body", NodeStatement, CNode>
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
        Capture<"body", NodeStatement, CNode>
      >
    >
  >;
  // clang-format on
};


using statement_switch =
Seq<
  Keyword<"switch">,
  Capture<"condition", NodeExpression, CNode>,
  Atom<'{'>,
  Any<
    Capture<"case", NodeStatementCase, CNode>,
    Capture<"default", NodeStatementDefault, CNode>
  >,
  Atom<'}'>
>;

struct NodeStatementSwitch : public CNode {};

//------------------------------------------------------------------------------

using statement_while =
Seq<
  Keyword<"while">,
  DelimitedList<
    Atom<'('>,
    Capture<"condition", NodeExpression, CNode>,
    Atom<','>,
    Atom<')'>
  >,
  Capture<"body", NodeStatement, CNode>
>;

struct NodeStatementWhile : public CNode {};

//------------------------------------------------------------------------------

using statement_dowhile =
Seq<
  Keyword<"do">,
  Capture<"body", NodeStatement, CNode>,
  Keyword<"while">,
  DelimitedList<
    Atom<'('>,
    Capture<"condition", NodeExpression, CNode>,
    Atom<','>,
    Atom<')'>
  >,
  Atom<';'>
>;

struct NodeStatementDoWhile : public CNode {};

//------------------------------------------------------------------------------

using statement_label =
Seq<
  Capture<"name", lit_identifier, NodeIdentifier>,
  Atom<':'>,
  Opt<Atom<';'>>
>;

struct NodeStatementLabel : public CNode {};

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
    Capture<"name", NodeAtom<LEX_STRING>, CNode>,
    Opt<Seq<
      Atom<'('>,
      Capture<"value", NodeExpression, CNode>,
      Atom<')'>
    >>
  >;
  // clang-format on
};

struct NodeAsmRefs : public CNode, public PatternWrapper<NodeAsmRefs> {
  using pattern =
  comma_separated<
    Capture<"ref", NodeAsmRef, CNode>
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

using statement_asm =
Seq<
  Oneof<
    Keyword<"asm">,
    Keyword<"__asm">,
    Keyword<"__asm__">
  >,
  Opt<Capture<"qualifiers", NodeAsmQualifiers, CNode>>,
  Atom<'('>,
  NodeAtom<LEX_STRING>,  // assembly code
  SeqOpt<
    // output operands
    Seq<Atom<':'>, Opt<Capture<"output", NodeAsmRefs, CNode>>>,
    // input operands
    Seq<Atom<':'>, Opt<Capture<"input", NodeAsmRefs, CNode>>>,
    // clobbers
    Seq<Atom<':'>, Opt<Capture<"clobbers", NodeAsmRefs, CNode>>>,
    // GotoLabels
    Seq<Atom<':'>, Opt<Capture<"gotos", comma_separated<lit_identifier>, CNode>>>
  >,
  Atom<')'>,
  Atom<';'>
>;

struct NodeStatementAsm : public CNode {};

//------------------------------------------------------------------------------

using def_typedef =
Seq<
  Opt<
    Keyword<"__extension__">
  >,
  Keyword<"typedef">,
  Capture<
    "newtype",
    Oneof<
      Capture<"struct", NodeStruct, CNode>,
      Capture<"union",  NodeUnion, CNode>,
      Capture<"class",  NodeClass, CNode>,
      Capture<"enum",   NodeEnum, CNode>,
      Capture<"decl",   NodeDeclaration, CNode>
    >
  , CNode>
>;

struct NodeTypedef : public CNode {

  static void extract_declarator(CContext& ctx, CNode* decl) {
    if (auto name = decl->child("name")) {
      if (auto id = name->child("identifier")) {
        ctx.add_typedef_type(id->span.begin);
      }
    }
  }

  static void extract_declarator_list(CContext& ctx, CNode* decls) {
    if (!decls) return;
    for (auto child = decls->child_head; child; child = child->node_next) {

      if (strcmp(child->match_tag, "decl") == 0) {
        extract_declarator(ctx, child);
      }
      else {
        matcheroni_assert(false);
      }
    }
  }

  static void extract_type(CContext& ctx) {
    auto node = ctx.top_tail;

    if (auto c = node->child("union")) {
      extract_declarator_list(ctx, c->child("decls"));
      return;
    }

    if (auto c = node->child("struct")) {
      extract_declarator_list(ctx, c->child("decls"));
      return;
    }

    if (auto c = node->child("class")) {
      extract_declarator_list(ctx, c->child("decls"));
      return;
    }

    if (auto c = node->child("enum")) {
      extract_declarator_list(ctx, c->child("decls"));
      return;
    }

    if (auto c = node->child("decl")) {
      extract_declarator_list(ctx, c->child("decls"));
      return;
    }

    matcheroni_assert(false);
  }

  static TokenSpan match(CContext& ctx, TokenSpan body) {
    auto tail = def_typedef::match(ctx, body);
    if (tail.is_valid()) {
      extract_type(ctx);
    }
    return tail;
  }
};

//------------------------------------------------------------------------------

// pr21356.c - Spec says goto should be an identifier, GCC allows expressions
using statement_goto = Seq<Keyword<"goto">, NodeExpression, Atom<';'>>;

struct NodeStatementGoto : public CNode {};

//------------------------------------------------------------------------------

struct NodeStatement : public PatternWrapper<NodeStatement> {
  // clang-format off
  using pattern =
  Oneof<
    // All of these have keywords or something first
    Capture<"class",    Seq<NodeClass,   Atom<';'>>, CNode>,
    Capture<"struct",   Seq<NodeStruct,  Atom<';'>>, CNode>,
    Capture<"union",    Seq<NodeUnion,   Atom<';'>>, CNode>,
    Capture<"enum",     Seq<NodeEnum,    Atom<';'>>, CNode>,
    Capture<"typedef",  Seq<Ref<NodeTypedef::match>, Atom<';'>>, NodeTypedef>,

    Capture<"for",      statement_for,     NodeStatementFor>,
    Capture<"if",       NodeStatementIf, CNode>,
    Capture<"return",   NodeStatementReturn, CNode>,
    Capture<"switch",   statement_switch,  NodeStatementSwitch>,
    Capture<"dowhile",  statement_dowhile, NodeStatementDoWhile>,
    Capture<"while",    statement_while,   NodeStatementWhile>,
    Capture<"goto",     statement_goto,    NodeStatementGoto>,
    Capture<"asm",      statement_asm,     NodeStatementAsm>,
    Capture<"compound", NodeStatementCompound, CNode>,
    Capture<"break",    NodeStatementBreak, CNode>,
    Capture<"continue", NodeStatementContinue, CNode>,

    // These don't - but they might confuse a keyword with an identifier...
    Capture<"label",    statement_label,     NodeStatementLabel>,
    Capture<"function", definition_function, NodeFunctionDefinition>,

    // If declaration is before expression, we parse "x = 1;" as a declaration
    // because it matches a declarator (bare identifier) + initializer list :/
    Capture<"expression",  Seq<comma_separated<NodeExpression>, Atom<';'>>, CNode>,
    Capture<"declaration", Seq<NodeDeclaration, Atom<';'>>, CNode>,

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
      Capture<"class",        Seq<NodeClass,  Atom<';'>>, CNode>,
      Capture<"struct",       Seq<NodeStruct, Atom<';'>>, CNode>,
      Capture<"union",        Seq<NodeUnion,  Atom<';'>>, CNode>,
      Capture<"enum",         Seq<NodeEnum,   Atom<';'>>, CNode>,
      Capture<"typedef",  Ref<NodeTypedef::match>, NodeTypedef>,
      Capture<"preproc",  NodePreproc, NodePreproc>,
      Capture<"template",     Seq<NodeTemplate, Atom<';'>>, CNode>,
      Capture<"function", definition_function, NodeFunctionDefinition>,
      Capture<"declaration",  Seq<NodeDeclaration, Atom<';'>>, CNode>,
      Capture<"namespace",    NodeNamespace, CNode>,
      Atom<';'>
    >
  >;
  // clang-format on
};

//------------------------------------------------------------------------------
