// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once

#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Utilities.hpp"

#include "examples/c99_parser/ParseNode.hpp"
#include "examples/c99_parser/Lexeme.hpp"
#include "examples/c99_parser/SST.hpp"
#include "examples/c99_parser/Trace.hpp"
#include "examples/c99_parser/c_constants.hpp"
#include "examples/c99_parser/C99Parser.hpp"

#include <assert.h>

using namespace matcheroni;

struct Token;

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
// You'll need to define atom_cmp(void* ctx, atom& a, StringParam<N>& b) to use
// this.

template <StringParam lit>
struct Keyword {
  static_assert(SST<c99_keywords>::contains(lit.str_val));

  tspan match(void* ctx, tspan s) {
    if (!s) return s.fail();
    if (atom_cmp(ctx, s.a, LEX_KEYWORD)) return s.fail();
    /*+*/ parser_rewind(ctx, s);
    if (atom_cmp(ctx, s.a, lit)) return s.fail();
    return s.advance(1);
  }
};

template <StringParam lit>
struct Literal2 {
  static tspan match(void* ctx, tspan s) {
    if (!s) return s.fail();
    if (atom_cmp(ctx, s.a, lit)) return s.fail();
    return s.advance(1);
  }
};

//------------------------------------------------------------------------------

#if 0

// Consumes spans from all tokens it matches with and creates a new node on top
// of them.

template <typename NodeType>
struct NodeMaker {
  static Token* match(void* ctx, Token* a, Token* b) {
    if (!a || a == b) return nullptr;

    print_trace_start<NodeType, Token>(a);
    auto end = NodeType::pattern::match(ctx, a, b);
    print_trace_end<NodeType, Token>(a, end);

    if (end && end != a) {
      auto node = new NodeType();
      node->init_node(ctx, a, end - 1, a->get_span(), (end - 1)->get_span());
    }
    return end;
  }
};

template <typename NodeType>
struct LeafMaker {
  static Token* match(void* ctx, Token* a, Token* b) {
    if (!a || a == b) return nullptr;

    print_trace_start<NodeType, Token>(a);
    auto end = NodeType::pattern::match(ctx, a, b);
    print_trace_end<NodeType, Token>(a, end);

    if (end && end != a) {
      auto node = new NodeType();
      node->init_leaf(ctx, a, end - 1);
    }
    return end;
  }
};
#endif

//------------------------------------------------------------------------------

template <StringParam lit>
inline tspan match_punct(void* ctx, tspan s) {
  if (!s) return s.fail();
  if (s.len() < lit.str_len) return s.fail();

  for (auto i = 0; i < lit.str_len; i++) {
    if (atom_cmp(ctx, s.a, LEX_PUNCT)) {
      return s.fail();
    }
    if (atom_cmp(ctx, s.a->lex->span.a, lit.str_val[i])) {
      return s.fail();
    }
    s.advance(1);
  }

  return s;
}

//------------------------------------------------------------------------------

template <auto P>
struct NodeAtom : public ParseNode {
  using pattern = Atom<P>;
};

template <StringParam lit>
struct NodeKeyword : public ParseNode {
  using pattern = Keyword<lit>;
};

template <StringParam lit>
struct NodeLiteral : public ParseNode {
  using pattern = Literal2<lit>;
};

//------------------------------------------------------------------------------
// Our builtin types are any sequence of prefixes followed by a builtin type

struct NodeBuiltinType : public ParseNode {
  using match_prefix = Ref<&C99Parser::match_builtin_type_prefix>;
  using match_base = Ref<&C99Parser::match_builtin_type_base>;
  using match_suffix = Ref<&C99Parser::match_builtin_type_suffix>;

  // clang-format off
  using pattern =
  Seq<
    Any<
      Seq<
        Capture<"prefix", match_prefix, ParseNode>,
        And<match_base>
      >
    >,
    Capture<"base", match_base, ParseNode>,
    Opt<Capture<"suffix", match_suffix, ParseNode>>
  >;
  // clang-format on
};

struct NodeTypedefType : public ParseNode {
  using pattern = Ref<&C99Parser::match_typedef_type>;
};


//------------------------------------------------------------------------------
// Excluding builtins and typedefs from being identifiers changes the total
// number of parse nodes, but why?

// - Because "uint8_t *x = 5" gets misparsed as an expression if uint8_t matches
// as an identifier

struct NodeIdentifier : public ParseNode {
  using pattern =
  Seq<
    Not<NodeBuiltinType>,
    Not<NodeTypedefType>,
    Atom<LEX_IDENTIFIER>
  >;
};

//------------------------------------------------------------------------------

struct NodePreproc : public ParseNode {
  using pattern = Atom<LEX_PREPROC>;
};

//------------------------------------------------------------------------------

struct NodeConstant : public ParseNode {
  using pattern =
  Oneof<
    Capture<"float",  Atom<LEX_FLOAT>,        ParseNode>,
    Capture<"int",    Atom<LEX_INT>,          ParseNode>,
    Capture<"char",   Atom<LEX_CHAR>,         ParseNode>,
    Capture<"string", Some<Atom<LEX_STRING>>, ParseNode>
  >;
};

//------------------------------------------------------------------------------

template <StringParam lit>
struct NodePrefixOp : public ParseNode {
  NodePrefixOp() {
    precedence = prefix_precedence(lit.str_val);
    assoc = prefix_assoc(lit.str_val);
  }

  using pattern = Ref<match_punct<lit>>;
};

//------------------------------------------------------------------------------

template <StringParam lit>
struct NodeBinaryOp : public ParseNode {
  NodeBinaryOp() {
    precedence = binary_precedence(lit.str_val);
    assoc = binary_assoc(lit.str_val);
  }

  using pattern = Ref<match_punct<lit>>;
};

//------------------------------------------------------------------------------

template <StringParam lit>
struct NodeSuffixOp : public ParseNode {
  NodeSuffixOp() {
    precedence = suffix_precedence(lit.str_val);
    assoc = suffix_assoc(lit.str_val);
  }

  using pattern = Ref<match_punct<lit>>;
};


//------------------------------------------------------------------------------

struct NodeQualifier : public ParseNode {
  static tspan match(void* ctx, tspan s) {
    assert(s.is_valid());
    auto span = s.a->lex->span;
    if (SST<qualifiers>::match(span.a, span.b)) {
      return s.advance(1);
    }
    else {
      return s.fail();
    }
  }
};


//------------------------------------------------------------------------------

struct NodeAsmSuffix : public ParseNode {
  using pattern =
  Seq<
    Oneof<
      Keyword<"asm">,
      Keyword<"__asm">,
      Keyword<"__asm__">
    >,
    Atom<'('>,
    Capture<"code", Some<NodeAtom<LEX_STRING>>, ParseNode>,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------


struct NodeAccessSpecifier : public ParseNode {
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

struct NodeParenType : public ParseNode {
  using pattern = Seq<Atom<'('>, NodeTypeName, Atom<')'>>;
};

struct NodePrefixCast : public ParseNode {
  NodePrefixCast() {
    precedence = 3;
    assoc = -2;
  }

  using pattern = Seq<Atom<'('>, NodeTypeName, Atom<')'>>;
};


//------------------------------------------------------------------------------

struct NodeExpressionParen : public ParseNode {
  using pattern =
      DelimitedList<Atom<'('>, NodeExpression, Atom<','>, Atom<')'>>;
};

struct NodeSuffixParen : public ParseNode {
  NodeSuffixParen() {
    precedence = 2;
    assoc = 2;
  }

  using pattern =
      DelimitedList<Atom<'('>, NodeExpression, Atom<','>, Atom<')'>>;
};

//------------------------------------------------------------------------------

struct NodeExpressionBraces : public ParseNode {
  using pattern =
      DelimitedList<Atom<'{'>, NodeExpression, Atom<','>, Atom<'}'>>;
};

struct NodeSuffixBraces : public ParseNode {
  NodeSuffixBraces() {
    precedence = 2;
    assoc = 2;
  }

  using pattern =
      DelimitedList<Atom<'{'>, NodeExpression, Atom<','>, Atom<'}'>>;
};


//------------------------------------------------------------------------------

struct NodeSuffixSubscript : public ParseNode {
  NodeSuffixSubscript() {
    precedence = 2;
    assoc = 2;
  }

  using pattern =
      DelimitedList<Atom<'['>, NodeExpression, Atom<','>, Atom<']'>>;
};

//------------------------------------------------------------------------------
// This is a weird ({...}) thing that GCC supports

struct NodeExpressionGccCompound : public ParseNode {
  using pattern =
  Seq<
    Opt<Keyword<"__extension__">>,
    Atom<'('>,
    NodeStatementCompound,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeExpressionTernary : public ParseNode {};
struct NodeExpressionBinary : public ParseNode {};
struct NodeExpressionPrefix : public ParseNode {};
struct NodeExpressionSuffix : public ParseNode {};

struct NodeExpressionSizeof : public ParseNode {
  using pattern =
  Seq<
    Keyword<"sizeof">,
    Oneof<
      NodeParenType,
      NodeExpressionParen,
      NodeExpression
    >
  >;
};

struct NodeExpressionAlignof : public ParseNode {
  using pattern =
      Seq<Keyword<"__alignof__">, Oneof<NodeParenType, NodeExpressionParen>>;
};

/*
struct NodeExpressionOffsetof : public ParseNode {
  using pattern =
  Seq<
    Oneof<
      Keyword<"offsetof">,
      Keyword<"__builtin_offsetof">
    >,
    Atom<'('>,
    Capture<"name", NodeTypeName::pattern, NodeTypeName>,
    Atom<','>,
    NodeExpression,
    Atom<')'>
  >;
};
*/

//----------------------------------------

template <StringParam lit>
struct NodePrefixKeyword : public ParseNode {
  NodePrefixKeyword() {
    precedence = 3;
    assoc = -2;
  }

  using pattern = Keyword<lit>;
};

// clang-format off
using ExpressionPrefixOp =
Oneof<
  NodePrefixCast,
  NodePrefixKeyword<"__extension__">,
  NodePrefixKeyword<"__real">,
  NodePrefixKeyword<"__real__">,
  NodePrefixKeyword<"__imag">,
  NodePrefixKeyword<"__imag__">,
  NodePrefixOp<"++">,
  NodePrefixOp<"--">,
  NodePrefixOp<"+">,
  NodePrefixOp<"-">,
  NodePrefixOp<"!">,
  NodePrefixOp<"~">,
  NodePrefixOp<"*">,
  NodePrefixOp<"&">
>;
// clang-format on

//----------------------------------------

// clang-format off
using ExpressionCore =
Oneof<
  NodeExpressionSizeof,
  NodeExpressionAlignof,
  // FIXME FIXME FIXME DO NOT RUN TESTS
  //NodeExpressionOffsetof,
  NodeExpressionGccCompound,
  NodeExpressionParen,
  NodeInitializerList,
  NodeExpressionBraces,
  NodeIdentifier,
  NodeConstant
>;
// clang-format on

//----------------------------------------

// clang-format off
using ExpressionSuffixOp =
Oneof<
  NodeSuffixInitializerList,  // must be before NodeSuffixBraces
  NodeSuffixBraces,
  NodeSuffixParen,
  NodeSuffixSubscript,
  NodeSuffixOp<"++">,
  NodeSuffixOp<"--">
>;
// clang-format on

//----------------------------------------

// clang-format off
using unit_pattern =
Seq<
  Any<ExpressionPrefixOp>,
  ExpressionCore,
  Any<ExpressionSuffixOp>
>;
// clang-format on

//----------------------------------------

struct NodeTernaryOp : public ParseNode {
  NodeTernaryOp() {
    precedence = 16;
    assoc = -1;
  }

  using pattern = Seq<NodeBinaryOp<"?">, Opt<comma_separated<NodeExpression>>,
                      NodeBinaryOp<":">>;
};

#if 0


//----------------------------------------

struct NodeExpression : public ParseNode {
  static Token* match_binary_op(void* ctx, Token* a, Token* b) {
    if (!a || a == b) return nullptr;

    if (atom_cmp(ctx, a, LEX_PUNCT)) {
      return nullptr;
    }
    /*+*/ parser_rewind(ctx, a, b);

    // clang-format off
    switch (a->unsafe_span_a()[0]) {
      case '+':
        return Oneof<NodeBinaryOp<"+=">, NodeBinaryOp<"+">>::match(ctx, a, b);
      case '-':
        return Oneof<NodeBinaryOp<"->*">, NodeBinaryOp<"->">, NodeBinaryOp<"-=">, NodeBinaryOp<"-">>::match(ctx, a, b);
      case '*':
        return Oneof<NodeBinaryOp<"*=">, NodeBinaryOp<"*">>::match(ctx, a, b);
      case '/':
        return Oneof<NodeBinaryOp<"/=">, NodeBinaryOp<"/">>::match(ctx, a, b);
      case '=':
        return Oneof<NodeBinaryOp<"==">, NodeBinaryOp<"=">>::match(ctx, a, b);
      case '<':
        return Oneof<NodeBinaryOp<"<<=">, NodeBinaryOp<"<=>">, NodeBinaryOp<"<=">, NodeBinaryOp<"<<">, NodeBinaryOp<"<">>::match(ctx, a, b);
      case '>':
        return Oneof<NodeBinaryOp<">>=">, NodeBinaryOp<">=">, NodeBinaryOp<">>">, NodeBinaryOp<">">>::match(ctx, a, b);
      case '!':
        return NodeBinaryOp<"!=">::match(ctx, a, b);
      case '&':
        return Oneof<NodeBinaryOp<"&&">, NodeBinaryOp<"&=">, NodeBinaryOp<"&">>::match(ctx, a, b);
      case '|':
        return Oneof<NodeBinaryOp<"||">, NodeBinaryOp<"|=">, NodeBinaryOp<"|">>::match(ctx, a, b);
      case '^':
        return Oneof<NodeBinaryOp<"^=">, NodeBinaryOp<"^">>::match(ctx, a, b);
      case '%':
        return Oneof<NodeBinaryOp<"%=">, NodeBinaryOp<"%">>::match(ctx, a, b);
      case '.':
        return Oneof<NodeBinaryOp<".*">, NodeBinaryOp<".">>::match(ctx, a, b);
      case '?':
        return NodeTernaryOp::match(ctx, a, b);

        // FIXME this is only for C++, and
        // case ':': return NodeBinaryOp<"::">::match(ctx, a, b);
        // default:  return nullptr;
    }
    // clang-format on

    return nullptr;
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

    using pattern =
        Seq<unit_pattern, Any<Seq<Ref<match_binary_op>, unit_pattern>>>;

    auto tok_a = a;
    auto tok_b = pattern::match(ctx, a, b);
    if (tok_b == nullptr) {
      return nullptr;
    }

    while (0) {
      {
        auto c = tok_a;
        while (c && c < tok_b) {
          c->get_span()->dump_tree(0, 1);
          c = c->step_right();
        }
        printf("\n");
      }

      auto c = tok_a;
      while (c && c->get_span()->precedence && c < tok_b) {
        c = c->step_right();
      }

      c->dump_token();

      // ran out of units?
      if (c->get_span()->precedence) break;

      auto l = c - 1;
      if (l && l >= tok_a) {
        if (l->get_span()->assoc == -2) {
          auto node = new NodeExpressionPrefix();
          node->init_node(ctx, l, c, l->get_span(), c->get_span());
          continue;
        }
      }

      break;
    }

    // dump_tree(n, 0, 0);

    // Fold up as many nodes based on precedence as we can
#if 0
    while(1) {
      ParseNode*    na = nullptr;
      NodeOpBinary* ox = nullptr;
      ParseNode*    nb = nullptr;
      NodeOpBinary* oy = nullptr;
      ParseNode*    nc = nullptr;

      nc = (cursor - 1)->get_span()->as_a<ParseNode>();
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
        DCHECK(ox->assoc == oy->assoc);

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
          CHECK(false);
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

      nc = (cursor - 1)->get_span()->as_a<ParseNode>();
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
      const Token* c = a;
      while(1) {
        //c->dump_token();
        if (auto s = c->get_span()) {
          dump_tree(s, 1, 0);
        }
        if (auto end = c->get_span()->tok_b()) {
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

    return tok_b;
  }

  //----------------------------------------

  static Token* match(void* ctx, Token* a, Token* b) {
    print_trace_start<NodeExpression, Token>(a);
    auto end = match2(ctx, a, b);
    if (end) {
      auto node = new NodeExpression();
      node->init_node(ctx, a, end - 1, a->get_span(), (end - 1)->get_span());
    }
    print_trace_end<NodeExpression, Token>(a, end);
    return end;
  }
};

//------------------------------------------------------------------------------
// 20010911-1.c - Attribute can be empty

struct NodeAttribute : public ParseNode, public NodeMaker<NodeAttribute> {
  // clang-format off
  using pattern =
  Seq<
    Oneof<
      Keyword<"__attribute__">,
      Keyword<"__attribute">
    >,
    DelimitedList<
      Seq<Atom<'('>, Atom<'('>>,
      Oneof<NodeExpression, Atom<LEX_KEYWORD>>,
      Atom<','>,
      Seq<Atom<')'>, Atom<')'>>
    >
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

struct NodeAlignas : public ParseNode, public NodeMaker<NodeAlignas> {
  // clang-format off
  using pattern =
  Seq<
    Keyword<"_Alignas">,
    Atom<'('>,
    Oneof<NodeTypeDecl, NodeConstant>,
    Atom<')'>
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

struct NodeDeclspec : public ParseNode, public NodeMaker<NodeDeclspec> {
  // clang-format off
  using pattern =
  Seq<
    Keyword<"__declspec">,
    Atom<'('>,
    NodeIdentifier,
    Atom<')'>
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

struct NodeModifier : public PatternWrapper<NodeModifier> {
  // This is the slowest matcher in the app, why?
  using pattern =
      Oneof<NodeAlignas, NodeDeclspec, NodeAttribute, NodeQualifier>;
};

//------------------------------------------------------------------------------

struct NodeTypeDecl : public ParseNode, public NodeMaker<NodeTypeDecl> {
  using pattern =
      Seq<Any<NodeModifier>, NodeSpecifier, Opt<NodeAbstractDeclarator>>;
};

//------------------------------------------------------------------------------

struct NodePointer : public ParseNode, public NodeMaker<NodePointer> {
  using pattern = Seq<Literal2<"*">, Any<Literal2<"*">, NodeModifier>>;
};

//------------------------------------------------------------------------------

struct NodeEllipsis : public ParseNode, public LeafMaker<NodeEllipsis> {
  using pattern = Ref<match_punct<"...">>;
};

//------------------------------------------------------------------------------

struct NodeParam : public ParseNode, public NodeMaker<NodeParam> {
  using pattern = Oneof<NodeEllipsis,
                        Seq<Any<NodeModifier>, NodeSpecifier, Any<NodeModifier>,
                            Opt<NodeDeclarator, NodeAbstractDeclarator>>,
                        NodeIdentifier>;
};

//------------------------------------------------------------------------------

struct NodeParamList : public ParseNode, public NodeMaker<NodeParamList> {
  using pattern = DelimitedList<Atom<'('>, NodeParam, Atom<','>, Atom<')'>>;
};

//------------------------------------------------------------------------------

struct NodeArraySuffix : public ParseNode, public NodeMaker<NodeArraySuffix> {
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

struct NodeTemplateArgs : public ParseNode, public NodeMaker<NodeTemplateArgs> {
  using pattern =
      DelimitedList<Atom<'<'>, NodeExpression, Atom<','>, Atom<'>'>>;
};

//------------------------------------------------------------------------------

struct NodeAtomicType : public ParseNode, public NodeMaker<NodeAtomicType> {
  using pattern = Seq<Keyword<"_Atomic">, Atom<'('>, NodeTypeDecl, Atom<')'>>;
};

//------------------------------------------------------------------------------

struct NodeSpecifier : public ParseNode, public NodeMaker<NodeSpecifier> {
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

struct NodeTypeName : public ParseNode {
  using pattern =
      Seq<Some<NodeSpecifier, NodeModifier>, Opt<NodeAbstractDeclarator>>;
};

//------------------------------------------------------------------------------
// Spec says the bit size can be any constant expression, but can we use just a
// constant or a paren-expression?

// (6.7.2.1) struct-declarator:
//   declarator
//   declaratoropt : constant-expression

struct NodeBitSuffix : public ParseNode, public NodeMaker<NodeBitSuffix> {
  using pattern = Seq<Atom<':'>, NodeExpression>;
};

//------------------------------------------------------------------------------

struct NodeAbstractDeclarator : public ParseNode,
                                public NodeMaker<NodeAbstractDeclarator> {
  using pattern = Seq<Opt<NodePointer>,
                      Opt<Seq<Atom<'('>, NodeAbstractDeclarator, Atom<')'>>>,
                      Any<NodeAttribute, NodeArraySuffix, NodeParamList>>;
};

//------------------------------------------------------------------------------

struct NodeDeclarator : public ParseNode, public NodeMaker<NodeDeclarator> {
  using pattern =
      Seq<Any<NodeAttribute, NodeModifier, NodePointer>,
          Oneof<NodeIdentifier, Seq<Atom<'('>, NodeDeclarator, Atom<')'>>>,
          Opt<NodeAsmSuffix>,
          Any<NodeBitSuffix, NodeAttribute, NodeArraySuffix, NodeParamList>>;
};

//------------------------------------------------------------------------------

struct NodeDeclaratorList : public ParseNode,
                            public NodeMaker<NodeDeclaratorList> {
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

struct NodeFieldList : public ParseNode, public NodeMaker<NodeFieldList> {
  using pattern = DelimitedBlock<Atom<'{'>, NodeField, Atom<'}'>>;
};

//------------------------------------------------------------------------------

struct NodeNamespace : public ParseNode, public NodeMaker<NodeNamespace> {
  using pattern =
      Seq<Keyword<"namespace">, Opt<NodeIdentifier>, Opt<NodeFieldList>>;
};

//------------------------------------------------------------------------------

struct NodeStructType : public ParseNode, public LeafMaker<NodeStructType> {
  using pattern = Ref<&C99Parser::match_struct_type>;
};

struct NodeStructTypeAdder : public NodeIdentifier {
  static Token* match(void* ctx, Token* a, Token* b) {
    if (auto end = NodeIdentifier::match(ctx, a, b)) {
      ((C99Parser*)ctx)->add_struct_type(a);
      return end;
    } else if (auto end = NodeTypedefType::match(ctx, a, b)) {
      // Already typedef'd
      return end;
    } else {
      return nullptr;
    }
  }
};

struct NodeStruct : public ParseNode, public NodeMaker<NodeStruct> {
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

struct NodeUnionType : public ParseNode {
  static Token* match(void* ctx, Token* a, Token* b) {
    auto p = ((C99Parser*)ctx);
    auto end = p->match_union_type(a, b);
    if (end) {
      auto node = new NodeUnionType();
      node->init_leaf(ctx, a, end - 1);
    }
    return end;
  }
};

struct NodeUnionTypeAdder : public NodeIdentifier {
  static Token* match(void* ctx, Token* a, Token* b) {
    if (auto end = NodeIdentifier::match(ctx, a, b)) {
      ((C99Parser*)ctx)->add_union_type(a);
      return end;
    } else if (auto end = NodeTypedefType::match(ctx, a, b)) {
      // Already typedef'd
      return end;
    } else {
      return nullptr;
    }
  }
};

struct NodeUnion : public ParseNode, public NodeMaker<NodeUnion> {
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

struct NodeClassType : public ParseNode, public LeafMaker<NodeClassType> {
  using pattern = Ref<&C99Parser::match_class_type>;
};

struct NodeClassTypeAdder : public NodeIdentifier {
  static Token* match(void* ctx, Token* a, Token* b) {
    if (auto end = NodeIdentifier::match(ctx, a, b)) {
      ((C99Parser*)ctx)->add_class_type(a);
      return end;
    } else if (auto end = NodeTypedefType::match(ctx, a, b)) {
      // Already typedef'd
      return end;
    } else {
      return nullptr;
    }
  }
};

struct NodeClass : public ParseNode, public NodeMaker<NodeClass> {
  // clang-format off
  using pattern =
  Seq<
    Any<NodeModifier>,
    NodeLiteral<"class">,
    Any<NodeAttribute>,
    Opt<NodeClassTypeAdder>,
    Opt<NodeFieldList>,
    Any<NodeAttribute>,
    Opt<NodeDeclaratorList>
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

struct NodeTemplateParams : public ParseNode,
                            public NodeMaker<NodeTemplateParams> {
  using pattern =
      DelimitedList<Atom<'<'>, NodeDeclaration, Atom<','>, Atom<'>'>>;
};

struct NodeTemplate : public ParseNode, public NodeMaker<NodeTemplate> {
  using pattern = Seq<NodeLiteral<"template">, NodeTemplateParams, NodeClass>;
};

//------------------------------------------------------------------------------
// FIXME should probably have a few diffeerent versions instead of all the opts

struct NodeEnumType : public ParseNode, public LeafMaker<NodeEnumType> {
  using pattern = Ref<&C99Parser::match_enum_type>;
};

struct NodeEnumTypeAdder : public NodeIdentifier {
  static Token* match(void* ctx, Token* a, Token* b) {
    if (auto end = NodeIdentifier::match(ctx, a, b)) {
      ((C99Parser*)ctx)->add_enum_type(a);
      return end;
    } else if (auto end = NodeTypedefType::match(ctx, a, b)) {
      // Already typedef'd
      return end;
    } else {
      return nullptr;
    }
  }
};

struct NodeEnumerator : public ParseNode, public NodeMaker<NodeEnumerator> {
  using pattern = Seq<NodeIdentifier, Opt<Seq<Atom<'='>, NodeExpression>>>;
};

struct NodeEnumerators : public ParseNode, public NodeMaker<NodeEnumerators> {
  using pattern =
      DelimitedList<Atom<'{'>, NodeEnumerator, Atom<','>, Atom<'}'>>;
};

struct NodeEnum : public ParseNode, public NodeMaker<NodeEnum> {
  // clang-format off
  using pattern =
  Seq<
    Any<NodeModifier>,
    Keyword<"enum">,
    Opt<NodeLiteral<"class">>,
    Opt<NodeEnumTypeAdder>,
    Opt<Seq<Atom<':'>, NodeTypeDecl>>,
    Opt<NodeEnumerators>,
    Opt<NodeDeclaratorList>
  >;
};

//------------------------------------------------------------------------------

struct NodeDesignation : public ParseNode, public NodeMaker<NodeDesignation> {
  // clang-format off
  using pattern =
  Some<
    Seq<Atom<'['>, NodeConstant, Atom<']'>>,
    Seq<Atom<'['>, NodeIdentifier, Atom<']'>>,
    Seq<Atom<'.'>, NodeIdentifier>
  >;
  // clang-format on
};

struct NodeInitializerList : public ParseNode, public NodeMaker<NodeInitializerList> {
  using pattern = DelimitedList<
      Atom<'{'>,
      Seq<Opt<Seq<NodeDesignation, Atom<'='>>,
              Seq<NodeIdentifier, Atom<':'>>  // This isn't in the C grammar but
                                              // compndlit-1.c uses it?
              >,
          NodeInitializer>,
      Atom<','>, Atom<'}'>>;
};

struct NodeSuffixInitializerList : public ParseNode, public NodeMaker<NodeSuffixInitializerList> {
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

struct NodeInitializer : public ParseNode, public NodeMaker<NodeInitializer> {
  using pattern = Oneof<NodeInitializerList, NodeExpression>;
};

//------------------------------------------------------------------------------

struct NodeFunctionIdentifier : public ParseNode,
                                public NodeMaker<NodeFunctionIdentifier> {
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

struct NodeFunctionDefinition : public ParseNode,
                                public NodeMaker<NodeFunctionDefinition> {
  // clang-format off
  using pattern =
  Seq<
    Any<NodeModifier>,
    Opt<NodeSpecifier>,
    Any<NodeModifier>,
    NodeFunctionIdentifier,
    NodeParamList,
    Any<NodeModifier>,
    Opt<NodeAsmSuffix>,
    Opt<NodeKeyword<"const">>,
    // This is old-style declarations after param list
    Opt<Some<Seq<NodeDeclaration, Atom<';'>>>>,
    NodeStatementCompound
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

struct NodeConstructor : public ParseNode, public NodeMaker<NodeConstructor> {
  // clang-format off
  using pattern =
  Seq<
    NodeClassType,
    NodeParamList,
    Oneof<
      Atom<';'>,
      NodeStatementCompound
    >
  >;
  // clang-format om
};

//------------------------------------------------------------------------------
// FIXME this is messy

struct NodeDeclaration : public ParseNode, public NodeMaker<NodeDeclaration> {
  // clang-format off
  using pattern =
  Seq<
    Any<NodeAttribute, NodeModifier>,
    Oneof<
      Seq<
        NodeSpecifier,
        Any<NodeAttribute, NodeModifier>,
        Opt<NodeDeclaratorList>
      >,
      Seq<
        Opt<NodeSpecifier>,
        Any<NodeAttribute, NodeModifier>,
        NodeDeclaratorList
      >
    >
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

template <typename P>
struct PushPopScope {
  static Token* match(void* ctx, Token* a, Token* b) {
    ((C99Parser*)ctx)->push_scope();

    auto end = P::match(ctx, a, b);

    ((C99Parser*)ctx)->pop_scope();

    return end;
  }
};

struct NodeStatementCompound : public ParseNode, public NodeMaker<NodeStatementCompound> {
  using pattern =
      PushPopScope<DelimitedBlock<Atom<'{'>, NodeStatement, Atom<'}'>>>;
};

//------------------------------------------------------------------------------

struct NodeStatementFor : public ParseNode, public NodeMaker<NodeStatementFor> {
  // clang-format off
  using pattern =
  Seq<
    Keyword<"for">,
    Atom<'('>,
    // This is _not_ the same as
    // Opt<Oneof<e, x>>, Atom<';'>
    Oneof<
      Seq<comma_separated<NodeExpression>,  Atom<';'>>,
      Seq<comma_separated<NodeDeclaration>, Atom<';'>>,
      Atom<';'>
    >,
    Opt<comma_separated<NodeExpression>>,
    Atom<';'>,
    Opt<comma_separated<NodeExpression>>,
    Atom<')'>,
    Oneof<NodeStatementCompound, NodeStatement>
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

struct NodeStatementElse : public ParseNode, public NodeMaker<NodeStatementElse> {
  using pattern = Seq<Keyword<"else">, NodeStatement>;
};

struct NodeStatementIf : public ParseNode, public NodeMaker<NodeStatementIf> {
  using pattern =
      Seq<Keyword<"if">,

          DelimitedList<Atom<'('>, NodeExpression, Atom<','>, Atom<')'>>,

          NodeStatement, Opt<NodeStatementElse>>;
};

//------------------------------------------------------------------------------

struct NodeStatementReturn : public ParseNode, public NodeMaker<NodeStatementReturn> {
  using pattern = Seq<Keyword<"return">, Opt<NodeExpression>, Atom<';'>>;
};

//------------------------------------------------------------------------------

struct NodeStatementCase : public ParseNode, public NodeMaker<NodeStatementCase> {
  // clang-format off
  using pattern =
  Seq<
    Keyword<"case">,
    NodeExpression,
    // case 1...2: - this is supported by GCC?
    Opt<Seq<NodeEllipsis, NodeExpression>>,
    Atom<':'>,
    Any<
      Seq<
        Not<Keyword<"case">>,
        Not<Keyword<"default">>,
        NodeStatement
      >
    >
  >;
  // clang-format on
};

struct NodeStatementDefault : public ParseNode, public NodeMaker<NodeStatementDefault> {
  // clang-format off
  using pattern =
  Seq<
    Keyword<"default">,
    Atom<':'>,
    Any<
      Seq<
        Not<Keyword<"case">>,
        Not<Keyword<"default">>,
        NodeStatement
      >
    >
  >;
  // clang-format on
};

struct NodeStatementSwitch : public ParseNode, public NodeMaker<NodeStatementSwitch> {
  // clang-format off
  using pattern =
  Seq<
    Keyword<"switch">,
    NodeExpression,
    Atom<'{'>,
    Any<NodeStatementCase, NodeStatementDefault>,
    Atom<'}'>
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

struct NodeStatementWhile : public ParseNode,
                            public NodeMaker<NodeStatementWhile> {
  // clang-format on
  using pattern =
  Seq<
    Keyword<"while">,
    DelimitedList<Atom<'('>, NodeExpression, Atom<','>, Atom<')'>>,
    NodeStatement
  >;
  // clang-format off
};

//------------------------------------------------------------------------------

struct NodeStatementDoWhile : public ParseNode,
                              public NodeMaker<NodeStatementDoWhile> {
  // clang-format off
  using pattern =
  Seq<
    Keyword<"do">,
    NodeStatement,
    Keyword<"while">,
    DelimitedList<Atom<'('>, NodeExpression, Atom<','>, Atom<')'>>,
    Atom<';'>
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

struct NodeStatementLabel : public ParseNode, public NodeMaker<NodeStatementLabel> {
  using pattern = Seq<NodeIdentifier, Atom<':'>, Opt<Atom<';'>>>;
};

//------------------------------------------------------------------------------

struct NodeStatementBreak : public ParseNode, public NodeMaker<NodeStatementBreak> {
  using pattern = Seq<Keyword<"break">, Atom<';'>>;
};

struct NodeStatementContinue : public ParseNode, public NodeMaker<NodeStatementContinue> {
  using pattern = Seq<Keyword<"continue">, Atom<';'>>;
};

//------------------------------------------------------------------------------

struct NodeAsmRef : public ParseNode, public NodeMaker<NodeAsmRef> {
  // clang-format off
  using pattern =
  Seq<
    NodeAtom<LEX_STRING>,
    Opt<Seq<
      Atom<'('>,
      NodeExpression,
      Atom<')'>
    >>
  >;
  // clang-format on
};

struct NodeAsmRefs : public ParseNode, public NodeMaker<NodeAsmRefs> {
  using pattern = comma_separated<NodeAsmRef>;
};

//------------------------------------------------------------------------------

struct NodeAsmQualifiers : public ParseNode, public NodeMaker<NodeAsmQualifiers> {
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

struct NodeStatementAsm : public ParseNode, public NodeMaker<NodeStatementAsm> {
  // clang-format off
  using pattern =
  Seq<
    Oneof<
      Keyword<"asm">,
      Keyword<"__asm">,
      Keyword<"__asm__">
    >,
    Opt<NodeAsmQualifiers>,
    Atom<'('>,
    NodeAtom<LEX_STRING>,  // assembly code
    SeqOpt<
      // output operands
      Seq<Atom<':'>, Opt<NodeAsmRefs>>,
      // input operands
      Seq<Atom<':'>, Opt<NodeAsmRefs>>,
      // clobbers
      Seq<Atom<':'>, Opt<NodeAsmRefs>>,
      // GotoLabels
      Seq<Atom<':'>, Opt<comma_separated<NodeIdentifier>>>
    >,
    Atom<')'>,
    Atom<';'>
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

struct NodeTypedef : public ParseNode, public NodeMaker<NodeTypedef> {
  // clang-format off
  using pattern =
  Seq<
    Opt<
      Keyword<"__extension__">
    >,
    Keyword<"typedef">,
    Oneof<
      NodeStruct,
      NodeUnion,
      NodeClass,
      NodeEnum,
      NodeDeclaration
    >
  >;
  // clang-format on

  static void extract_declarator(void* ctx, NodeDeclarator* decl) {
    if (auto id = decl->child<NodeIdentifier>()) {
      ((C99Parser*)ctx)->add_typedef_type(id->tok_a());
    }

    for (auto child : decl) {
      if (auto decl = child->as_a<NodeDeclarator>()) {
        extract_declarator(ctx, decl);
      }
    }
  }

  static void extract_declarator_list(void* ctx, NodeDeclaratorList* decls) {
    if (!decls) return;
    for (auto child : decls) {
      if (auto decl = child->as_a<NodeDeclarator>()) {
        extract_declarator(ctx, decl);
      }
    }
  }

  static void extract_type(void* ctx, Token* a, Token* b) {
    auto node = a->get_span();

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

    CHECK(false);
  }

  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = NodeMaker::match(ctx, a, b);
    if (end) extract_type(ctx, a, b);
    return end;
  }
};

//------------------------------------------------------------------------------

struct NodeStatementGoto : public ParseNode, public NodeMaker<NodeStatementGoto> {
  // pr21356.c - Spec says goto should be an identifier, GCC allows expressions
  using pattern = Seq<Keyword<"goto">, NodeExpression, Atom<';'>>;
};

//------------------------------------------------------------------------------

struct NodeStatement : public PatternWrapper<NodeStatement> {
  // clang-format off
  using pattern =
  Oneof<
    // All of these have keywords or something first
    Seq<NodeClass,   Atom<';'>>,
    Seq<NodeStruct,  Atom<';'>>,
    Seq<NodeUnion,   Atom<';'>>,
    Seq<NodeEnum,    Atom<';'>>,
    Seq<NodeTypedef, Atom<';'>>,

    NodeStatementFor,
    NodeStatementIf,
    NodeStatementReturn,
    NodeStatementSwitch,
    NodeStatementDoWhile,
    NodeStatementWhile,
    NodeStatementGoto,
    NodeStatementAsm,
    NodeStatementCompound,
    NodeStatementBreak,
    NodeStatementContinue,

    // These don't - but they might confuse a keyword with an identifier...
    NodeStatementLabel,
    NodeFunctionDefinition,

    // If declaration is before expression, we parse "x = 1;" as a declaration
    // because it matches a declarator (bare identifier) + initializer list :/
    Seq<comma_separated<NodeExpression>, Atom<';'>>,
    Seq<NodeDeclaration, Atom<';'>>,

    // Extra semicolons
    Atom<';'>
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

struct NodeTranslationUnit : public ParseNode, public NodeMaker<NodeTranslationUnit> {
  // clang-format off
  using pattern =
  Any<
    Oneof<
      Seq<NodeClass,  Atom<';'>>,
      Seq<NodeStruct, Atom<';'>>,
      Seq<NodeUnion,  Atom<';'>>,
      Seq<NodeEnum,   Atom<';'>>,
      NodeTypedef,
      NodePreproc,
      Seq<NodeTemplate, Atom<';'>>,
      NodeFunctionDefinition,
      Seq<NodeDeclaration, Atom<';'>>,
      Atom<';'>
    >
  >;
  // clang-format on
};

//------------------------------------------------------------------------------

#endif
