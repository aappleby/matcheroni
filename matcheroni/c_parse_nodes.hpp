#pragma once

#include "Lexeme.hpp"

struct Token;

inline int  atom_cmp   (void* ctx, Token* a, LexemeType b);
inline int  atom_cmp   (void* ctx, Token* a, char b);
inline int  atom_cmp   (void* ctx, Token* a, const char* b);
inline int  atom_cmp   (void* ctx, Token* a, const Token* b);
inline void atom_rewind(void* ctx, Token* a, Token* b);

/*
template<int N>
inline int atom_cmp(void* ctx, Token* a, const StringParam<N>& b) {
  return ((C99Parser*)ctx)->atom_cmp(a, b);
}
*/

#if 0
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
// Matches string literals as if they were atoms. Does ___NOT___ match the
// trailing null.
// You'll need to define atom_cmp(void* ctx, atom& a, StringParam<N>& b) to use this.

template<StringParam lit>
struct Keyword {
  static_assert(SST<c99_keywords>::contains(lit.str_val));

  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    if (atom_cmp(ctx, a, LEX_KEYWORD)) return nullptr;
    /*+*/atom_rewind(ctx, a, b);
    if (atom_cmp(ctx, a, lit)) return nullptr;
    return a + 1;
  }
};

template<StringParam lit>
struct Literal2 {
  template<typename atom>
  static atom* match(void* ctx, atom* a, atom* b) {
    if (!a || a == b) return nullptr;
    if (atom_cmp(ctx, a, lit)) return nullptr;
    return a + 1;
  }
};

//------------------------------------------------------------------------------
// Consumes spans from all tokens it matches with and creates a new node on top of them.

template<typename NodeType>
struct NodeMaker {
  static Token* match(void* ctx, Token* a, Token* b) {
    if (!a || a == b) return nullptr;

    print_trace_start<NodeType, Token>(a);
    auto end = NodeType::pattern::match(ctx, a, b);
    print_trace_end<NodeType, Token>(a, end);

    if (end && end != a) {
      auto node = new NodeType();
      node->init_node(ctx, a, end-1, a->get_span(), (end-1)->get_span());
    }
    return end;
  }
};

template<typename NodeType>
struct LeafMaker {
  static Token* match(void* ctx, Token* a, Token* b) {
    if (!a || a == b) return nullptr;

    print_trace_start<NodeType, Token>(a);
    auto end = NodeType::pattern::match(ctx, a, b);
    print_trace_end<NodeType, Token>(a, end);

    if (end && end != a) {
      auto node = new NodeType();
      node->init_leaf(ctx, a, end-1);
    }
    return end;
  }
};

//------------------------------------------------------------------------------

template<StringParam lit>
inline Token* match_punct(void* ctx, Token* a, Token* b) {
  if (!a || a == b) return nullptr;
  if (a + lit.str_len > b) return nullptr;

  for (auto i = 0; i < lit.str_len; i++) {
    if (atom_cmp(ctx, a + i, LEX_PUNCT)) {
      return nullptr;
    }
    if (atom_cmp(ctx, a->unsafe_span_a() + i, lit.str_val[i])) {
      return nullptr;
    }
  }

  auto end = a + lit.str_len;
  return end;
}


//------------------------------------------------------------------------------

template<auto P>
struct NodeAtom : public ParseNode, public LeafMaker<NodeAtom<P>> {
  using pattern = Atom<P>;
};

template<StringParam lit>
struct NodeKeyword : public ParseNode, public LeafMaker<NodeKeyword<lit>> {
  using pattern = Keyword<lit>;
};

template<StringParam lit>
struct NodeLiteral : public ParseNode, public LeafMaker<NodeLiteral<lit>> {
  using pattern = Literal2<lit>;
};

//------------------------------------------------------------------------------
// Our builtin types are any sequence of prefixes followed by a builtin type

struct NodeBuiltinType : public ParseNode, public LeafMaker<NodeBuiltinType> {
  using match_prefix = Ref<&C99Parser::match_builtin_type_prefix>;
  using match_base   = Ref<&C99Parser::match_builtin_type_base>;
  using match_suffix = Ref<&C99Parser::match_builtin_type_suffix>;

  using pattern = Seq<
    Any<Seq<match_prefix, And<match_base>>>,
    match_base,
    Opt<match_suffix>
  >;
};

struct NodeTypedefType : public ParseNode , public LeafMaker<NodeTypedefType> {
  using pattern = Ref<&C99Parser::match_typedef_type>;
};


//------------------------------------------------------------------------------
// Excluding builtins and typedefs from being identifiers changes the total
// number of parse nodes, but why?

// - Because "uint8_t *x = 5" gets misparsed as an expression if uint8_t matches
// as an identifier

struct NodeIdentifier : public ParseNode, public LeafMaker<NodeIdentifier> {
  using pattern =
  Seq<
    Not<NodeBuiltinType>,
    Not<NodeTypedefType>,
    Atom<LEX_IDENTIFIER>
  >;
};

//------------------------------------------------------------------------------

struct NodePreproc : public ParseNode, public LeafMaker<NodePreproc> {
  using pattern = Atom<LEX_PREPROC>;
};

//------------------------------------------------------------------------------

struct NodeConstant : public ParseNode, public LeafMaker<NodeConstant> {
  using pattern = Oneof<
    Atom<LEX_FLOAT>,
    Atom<LEX_INT>,
    Atom<LEX_CHAR>,
    Some<Atom<LEX_STRING>>
  >;
};

//------------------------------------------------------------------------------

template<typename P>
using comma_separated = Seq<P, Any<Seq<Atom<','>, P>>, Opt<Atom<','>> >;

template<typename P>
using opt_comma_separated = Opt<comma_separated<P>>;

//------------------------------------------------------------------------------












//------------------------------------------------------------------------------

template<StringParam lit>
struct NodePrefixOp : public ParseNode, public LeafMaker<NodePrefixOp<lit>> {
  NodePrefixOp() {
    precedence = prefix_precedence(lit.str_val);
    assoc      = prefix_assoc(lit.str_val);
  }

  using pattern = Ref<match_punct<lit>>;
};

//------------------------------------------------------------------------------

template<StringParam lit>
struct NodeBinaryOp : public ParseNode, public LeafMaker<NodeBinaryOp<lit>> {
  NodeBinaryOp() {
    precedence = binary_precedence(lit.str_val);
    assoc      = binary_assoc(lit.str_val);
  }

  using pattern = Ref<match_punct<lit>>;
};

//------------------------------------------------------------------------------

template<StringParam lit>
struct NodeSuffixOp : public ParseNode, public LeafMaker<NodeSuffixOp<lit>> {
  NodeSuffixOp() {
    precedence = suffix_precedence(lit.str_val);
    assoc      = suffix_assoc(lit.str_val);
  }

  using pattern = Ref<match_punct<lit>>;
};

//------------------------------------------------------------------------------

struct NodeQualifier : public ParseNode {
  static Token* match(void* ctx, Token* a, Token* b) {
    if (!a || a == b) return nullptr;
    if (SST<qualifiers>::match(ctx, a, b)) {
      auto node = new NodeQualifier();
      node->init_leaf(ctx, a, a);
      return a + 1;
    }
    else {
      return nullptr;
    }
  }
};

//------------------------------------------------------------------------------

struct NodeAsmSuffix : public ParseNode, public NodeMaker<NodeAsmSuffix> {
  using pattern = Seq<
    Oneof<
      Keyword<"asm">,
      Keyword<"__asm">,
      Keyword<"__asm__">
    >,
    Atom<'('>,
    Some<NodeAtom<LEX_STRING>>,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeAccessSpecifier : public ParseNode, public LeafMaker<NodeAccessSpecifier> {
  using pattern = Seq<
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

struct NodeParenType : public ParseNode, public NodeMaker<NodeParenType> {
  using pattern = Seq<
    Atom<'('>,
    NodeTypeName,
    Atom<')'>
  >;
};

struct NodePrefixCast : public ParseNode, public NodeMaker<NodePrefixCast> {
  NodePrefixCast() {
    precedence = 3;
    assoc = -2;
  }

  using pattern = Seq<
    Atom<'('>,
    NodeTypeName,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeExpressionParen : public ParseNode, public NodeMaker<NodeExpressionParen> {
  using pattern =
  DelimitedList<
    Atom<'('>,
    NodeExpression,
    Atom<','>,
    Atom<')'>
  >;
};

struct NodeSuffixParen : public ParseNode, public NodeMaker<NodeSuffixParen> {
  NodeSuffixParen() {
    precedence = 2;
    assoc = 2;
  }

  using pattern =
  DelimitedList<
    Atom<'('>,
    NodeExpression,
    Atom<','>,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeExpressionBraces : public ParseNode, public NodeMaker<NodeExpressionBraces> {
  using pattern =
  DelimitedList<
    Atom<'{'>,
    NodeExpression,
    Atom<','>,
    Atom<'}'>
  >;
};

struct NodeSuffixBraces : public ParseNode, public NodeMaker<NodeSuffixBraces> {
  NodeSuffixBraces() {
    precedence = 2;
    assoc = 2;
  }

  using pattern =
  DelimitedList<
    Atom<'{'>,
    NodeExpression,
    Atom<','>,
    Atom<'}'>
  >;
};

//------------------------------------------------------------------------------

struct NodeSuffixSubscript : public ParseNode, public NodeMaker<NodeSuffixSubscript> {
  NodeSuffixSubscript() {
    precedence = 2;
    assoc = 2;
  }

  using pattern =
  DelimitedList<
    Atom<'['>,
    NodeExpression,
    Atom<','>,
    Atom<']'>
  >;
};

//------------------------------------------------------------------------------
// This is a weird ({...}) thing that GCC supports

struct NodeExpressionGccCompound : public ParseNode, public NodeMaker<NodeExpressionGccCompound> {
  using pattern = Seq<
    Opt<Keyword<"__extension__">>,
    Atom<'('>,
    NodeStatementCompound,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeExpressionTernary : public ParseNode {};
struct NodeExpressionBinary  : public ParseNode {};
struct NodeExpressionPrefix  : public ParseNode {};
struct NodeExpressionSuffix  : public ParseNode {};

struct NodeExpressionSizeof  : public ParseNode, public NodeMaker<NodeExpressionSizeof> {
  using pattern = Seq<
    Keyword<"sizeof">,
    Oneof<
      NodeParenType,
      NodeExpressionParen,
      NodeExpression
    >
  >;
};

struct NodeExpressionAlignof  : public ParseNode, public NodeMaker<NodeExpressionAlignof> {
  using pattern = Seq<
    Keyword<"__alignof__">,
    Oneof<
      NodeParenType,
      NodeExpressionParen
    >
  >;
};

struct NodeExpressionOffsetof  : public ParseNode, public NodeMaker<NodeExpressionOffsetof> {
  using pattern = Seq<
    Oneof<
      Keyword<"offsetof">,
      Keyword<"__builtin_offsetof">
    >,
    Atom<'('>,
    NodeTypeName,
    Atom<','>,
    NodeExpression,
    Atom<')'>
  >;
};

//----------------------------------------

template<StringParam lit>
struct NodePrefixKeyword : public ParseNode, public LeafMaker<NodePrefixKeyword<lit>> {
  NodePrefixKeyword() {
    precedence = 3;
    assoc = -2;
  }

  using pattern = Keyword<lit>;
};

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

//----------------------------------------

using ExpressionCore = Oneof<
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

using ExpressionSuffixOp =
Oneof<
  NodeSuffixInitializerList,   // must be before NodeSuffixBraces
  NodeSuffixBraces,
  NodeSuffixParen,
  NodeSuffixSubscript,
  NodeSuffixOp<"++">,
  NodeSuffixOp<"--">
>;

//----------------------------------------

using unit_pattern = Seq<
  Any<ExpressionPrefixOp>,
  ExpressionCore,
  Any<ExpressionSuffixOp>
>;

struct NodeTernaryOp : public ParseNode, public NodeMaker<NodeTernaryOp> {
  NodeTernaryOp() {
    precedence = 16;
    assoc = -1;
  }

  using pattern =
  Seq<
    NodeBinaryOp<"?">,
    Opt<comma_separated<NodeExpression>>,
    NodeBinaryOp<":">
  >;
};


//----------------------------------------

struct NodeExpression : public ParseNode {

  static Token* match_binary_op(void* ctx, Token* a, Token* b) {
    if (!a || a == b) return nullptr;

    if (atom_cmp(ctx, a, LEX_PUNCT)) {
      return nullptr;
    }
    /*+*/atom_rewind(ctx, a, b);

    switch(a->unsafe_span_a()[0]) {
      case '+': return Oneof< NodeBinaryOp<"+=">, NodeBinaryOp<"+"> >::match(ctx, a, b);
      case '-': return Oneof< NodeBinaryOp<"->*">, NodeBinaryOp<"->">, NodeBinaryOp<"-=">, NodeBinaryOp<"-"> >::match(ctx, a, b);
      case '*': return Oneof< NodeBinaryOp<"*=">, NodeBinaryOp<"*"> >::match(ctx, a, b);
      case '/': return Oneof< NodeBinaryOp<"/=">, NodeBinaryOp<"/"> >::match(ctx, a, b);
      case '=': return Oneof< NodeBinaryOp<"==">, NodeBinaryOp<"="> >::match(ctx, a, b);
      case '<': return Oneof< NodeBinaryOp<"<<=">, NodeBinaryOp<"<=>">, NodeBinaryOp<"<=">, NodeBinaryOp<"<<">, NodeBinaryOp<"<"> >::match(ctx, a, b);
      case '>': return Oneof< NodeBinaryOp<">>=">, NodeBinaryOp<">=">, NodeBinaryOp<">>">, NodeBinaryOp<">"> >::match(ctx, a, b);
      case '!': return NodeBinaryOp<"!=">::match(ctx, a, b);
      case '&': return Oneof< NodeBinaryOp<"&&">, NodeBinaryOp<"&=">, NodeBinaryOp<"&"> >::match(ctx, a, b);
      case '|': return Oneof< NodeBinaryOp<"||">, NodeBinaryOp<"|=">, NodeBinaryOp<"|"> >::match(ctx, a, b);
      case '^': return Oneof< NodeBinaryOp<"^=">, NodeBinaryOp<"^"> >::match(ctx, a, b);
      case '%': return Oneof< NodeBinaryOp<"%=">, NodeBinaryOp<"%"> >::match(ctx, a, b);
      case '.': return Oneof< NodeBinaryOp<".*">, NodeBinaryOp<"."> >::match(ctx, a, b);
      case '?': return NodeTernaryOp::match(ctx, a, b);

      // FIXME this is only for C++, and
      //case ':': return NodeBinaryOp<"::">::match(ctx, a, b);
      //default:  return nullptr;
    }

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
    //Token* cursor = a;
    //cursor = NodeExpressionUnit::match(ctx, a, b);
    /*
    cursor = unit_pattern::match(ctx, a, b);
    if (!cursor) return nullptr;

    using binary_pattern = Seq<Ref<match_binary_op>, unit_pattern>;

    while (auto end = binary_pattern::match(ctx, cursor, b)) {
      cursor = end;
    }
    */

    using pattern =
    Seq<
      unit_pattern,
      Any<Seq<
        Ref<match_binary_op>,
        unit_pattern
      >>
    >;

    auto tok_a = a;
    auto tok_b = pattern::match(ctx, a, b);
    if (tok_b == nullptr) {
      return nullptr;
    }


    while(0) {

      {
        auto c = tok_a;
        while(c && c < tok_b) {
          dump_tree(c->get_span(), 0, 1);
          c = c->step_right();
        }
        printf("\n");
      }

      auto c = tok_a;
      while(c && c->get_span()->precedence && c < tok_b) {
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

      /*
      auto r = c->step_right();
      if (r && r < tok_b) {
      }
      */

      break;
    }


    //dump_tree(n, 0, 0);




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
      node->init_node(ctx, a, end - 1, a->get_span(), (end-1)->get_span());
    }
    print_trace_end<NodeExpression, Token>(a, end);
    return end;
  }

};

//------------------------------------------------------------------------------
// 20010911-1.c - Attribute can be empty

struct NodeAttribute : public ParseNode, public NodeMaker<NodeAttribute> {
  using pattern = Seq<
    Oneof<
      Keyword<"__attribute__">,
      Keyword<"__attribute">
    >,
    DelimitedList<
      Seq<Atom<'('>, Atom<'('>>,
      Oneof<
        NodeExpression,
        Atom<LEX_KEYWORD>
      >,
      Atom<','>,
      Seq<Atom<')'>, Atom<')'>>
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeAlignas : public ParseNode, public NodeMaker<NodeAlignas> {
  using pattern = Seq<
    Keyword<"_Alignas">,
    Atom<'('>,
    Oneof<
      NodeTypeDecl,
      NodeConstant
    >,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeDeclspec : public ParseNode, public NodeMaker<NodeDeclspec> {
  using pattern = Seq<
    Keyword<"__declspec">,
    Atom<'('>,
    NodeIdentifier,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeModifier : public PatternWrapper<NodeModifier> {
  // This is the slowest matcher in the app, why?
  using pattern = Oneof<
    NodeAlignas,
    NodeDeclspec,
    NodeAttribute,
    NodeQualifier
  >;
};

//------------------------------------------------------------------------------

struct NodeTypeDecl : public ParseNode, public NodeMaker<NodeTypeDecl> {
  using pattern = Seq<
    Any<NodeModifier>,
    NodeSpecifier,
    Opt<NodeAbstractDeclarator>
  >;
};

//------------------------------------------------------------------------------

struct NodePointer : public ParseNode, public NodeMaker<NodePointer> {
  using pattern =
  Seq<
    Literal2<"*">,
    Any<
      Literal2<"*">,
      NodeModifier
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeEllipsis : public ParseNode, public LeafMaker<NodeEllipsis> {
  using pattern = Ref<match_punct<"...">>;
};

//------------------------------------------------------------------------------

struct NodeParam : public ParseNode, public NodeMaker<NodeParam> {
  using pattern = Oneof<
    NodeEllipsis,
    Seq<
      Any<NodeModifier>,
      NodeSpecifier,
      Any<NodeModifier>,
      Opt<
        NodeDeclarator,
        NodeAbstractDeclarator
      >
    >,
    NodeIdentifier
  >;
};

//------------------------------------------------------------------------------

struct NodeParamList : public ParseNode, public NodeMaker<NodeParamList> {
  using pattern =
  DelimitedList<
    Atom<'('>,
    NodeParam,
    Atom<','>,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeArraySuffix : public ParseNode, public NodeMaker<NodeArraySuffix> {
  using pattern = Oneof<
    Seq<Atom<'['>,                         Any<NodeModifier>,                           Opt<NodeExpression>, Atom<']'>>,
    Seq<Atom<'['>, NodeKeyword<"static">,  Any<NodeModifier>,                               NodeExpression,  Atom<']'>>,
    Seq<Atom<'['>,                         Any<NodeModifier>,   NodeKeyword<"static">,      NodeExpression,  Atom<']'>>,
    Seq<Atom<'['>,                         Any<NodeModifier>,   Literal2<"*">,                               Atom<']'>>
  >;
};

//------------------------------------------------------------------------------

struct NodeTemplateArgs : public ParseNode, public NodeMaker<NodeTemplateArgs> {
  using pattern =
  DelimitedList<
    Atom<'<'>,
    NodeExpression,
    Atom<','>,
    Atom<'>'>
  >;
};

//------------------------------------------------------------------------------

struct NodeAtomicType : public ParseNode, public NodeMaker<NodeAtomicType> {
  using pattern = Seq<
    Keyword<"_Atomic">,
    Atom<'('>,
    NodeTypeDecl,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeSpecifier : public ParseNode, public NodeMaker<NodeSpecifier> {
  using pattern = Seq<
    Oneof<
      // These have to be NodeIdentifier because "void foo(struct S);" is valid
      // even without the definition of S.
      Seq<NodeLiteral<"class">,  Oneof<NodeIdentifier, NodeTypedefType>>,
      Seq<Keyword<"union">,  Oneof<NodeIdentifier, NodeTypedefType>>,
      Seq<Keyword<"struct">, Oneof<NodeIdentifier, NodeTypedefType>>,
      Seq<Keyword<"enum">,   Oneof<NodeIdentifier, NodeTypedefType>>,

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
};

//------------------------------------------------------------------------------
// (6.7.6) type-name:
//   specifier-qualifier-list abstract-declaratoropt

struct NodeTypeName : public ParseNode, public NodeMaker<NodeTypeName> {
  using pattern = Seq<
    Some<
      NodeSpecifier,
      NodeModifier
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

struct NodeBitSuffix : public ParseNode, public NodeMaker<NodeBitSuffix> {
  using pattern = Seq< Atom<':'>, NodeExpression >;
};

//------------------------------------------------------------------------------

struct NodeAbstractDeclarator : public ParseNode, public NodeMaker<NodeAbstractDeclarator> {
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

struct NodeDeclarator : public ParseNode, public NodeMaker<NodeDeclarator> {
  using pattern = Seq<
    Any<
      NodeAttribute,
      NodeModifier,
      NodePointer
    >,
    Oneof<
      NodeIdentifier,
      Seq<Atom<'('>, NodeDeclarator, Atom<')'>>
    >,
    Opt<NodeAsmSuffix>,
    Any<
      NodeBitSuffix,
      NodeAttribute,
      NodeArraySuffix,
      NodeParamList
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeDeclaratorList : public ParseNode, public NodeMaker<NodeDeclaratorList> {
  using pattern =
  comma_separated<
    Seq<
      Oneof<
        Seq<NodeDeclarator, Opt<NodeBitSuffix> >,
        NodeBitSuffix
      >,
      Opt<Seq<Atom<'='>, NodeInitializer>>
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeField : public PatternWrapper<NodeField> {
  using pattern = Oneof<
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
};

struct NodeFieldList : public ParseNode, public NodeMaker<NodeFieldList> {
  using pattern =
  DelimitedBlock<
    Atom<'{'>,
    NodeField,
    Atom<'}'>
  >;
};

//------------------------------------------------------------------------------

struct NodeNamespace : public ParseNode, public NodeMaker<NodeNamespace> {
  using pattern = Seq<
    Keyword<"namespace">,
    Opt<NodeIdentifier>,
    Opt<NodeFieldList>
  >;
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
    }
    else if (auto end = NodeTypedefType::match(ctx, a, b)) {
      // Already typedef'd
      return end;
    }
    else {
      return nullptr;
    }
  }
};

struct NodeStruct : public ParseNode, public NodeMaker<NodeStruct> {
  using pattern = Seq<
    Any<NodeModifier>,
    Keyword<"struct">,
    Any<NodeAttribute>,    // This has to be here, there are a lot of struct __attrib__() foo {};
    Opt<NodeStructTypeAdder>,
    Opt<NodeFieldList>,
    Any<NodeAttribute>,
    Opt<NodeDeclaratorList>
  >;
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
    }
    else if (auto end = NodeTypedefType::match(ctx, a, b)) {
      // Already typedef'd
      return end;
    }
    else {
      return nullptr;
    }
  }
};

struct NodeUnion : public ParseNode, public NodeMaker<NodeUnion> {
  using pattern = Seq<
    Any<NodeModifier>,
    Keyword<"union">,
    Any<NodeAttribute>,
    Opt<NodeUnionTypeAdder>,
    Opt<NodeFieldList>,
    Any<NodeAttribute>,
    Opt<NodeDeclaratorList>
  >;
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
    }
    else if (auto end = NodeTypedefType::match(ctx, a, b)) {
      // Already typedef'd
      return end;
    }
    else {
      return nullptr;
    }
  }
};

struct NodeClass : public ParseNode, public NodeMaker<NodeClass> {
  using pattern = Seq<
    Any<NodeModifier>,
    NodeLiteral<"class">,
    Any<NodeAttribute>,
    Opt<NodeClassTypeAdder>,
    Opt<NodeFieldList>,
    Any<NodeAttribute>,
    Opt<NodeDeclaratorList>
  >;
};

//------------------------------------------------------------------------------

struct NodeTemplateParams : public ParseNode, public NodeMaker<NodeTemplateParams> {
  using pattern =
  DelimitedList<
    Atom<'<'>,
    NodeDeclaration,
    Atom<','>,
    Atom<'>'>
  >;
};

struct NodeTemplate : public ParseNode, public NodeMaker<NodeTemplate> {
  using pattern = Seq<
    NodeLiteral<"template">,
    NodeTemplateParams,
    NodeClass
  >;
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
    }
    else if (auto end = NodeTypedefType::match(ctx, a, b)) {
      // Already typedef'd
      return end;
    }
    else {
      return nullptr;
    }
  }
};

struct NodeEnumerator : public ParseNode, public NodeMaker<NodeEnumerator> {
  using pattern = Seq<
    NodeIdentifier,
    Opt<Seq<Atom<'='>, NodeExpression>>
  >;
};

struct NodeEnumerators : public ParseNode, public NodeMaker<NodeEnumerators> {
  using pattern =
  DelimitedList<
    Atom<'{'>,
    NodeEnumerator,
    Atom<','>,
    Atom<'}'>
  >;
};

struct NodeEnum : public ParseNode, public NodeMaker<NodeEnum> {
  using pattern = Seq<
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
  using pattern =
  Some<
    Seq<Atom<'['>, NodeConstant,   Atom<']'>>,
    Seq<Atom<'['>, NodeIdentifier, Atom<']'>>,
    Seq<Atom<'.'>, NodeIdentifier>
  >;
};

struct NodeInitializerList : public ParseNode, public NodeMaker<NodeInitializerList> {
  using pattern =
  DelimitedList<
    Atom<'{'>,
    Seq<
      Opt<
        Seq<NodeDesignation, Atom<'='>>,
        Seq<NodeIdentifier,  Atom<':'>> // This isn't in the C grammar but compndlit-1.c uses it?
      >,
      NodeInitializer
    >,
    Atom<','>,
    Atom<'}'>
  >;
};

struct NodeSuffixInitializerList : public ParseNode, public NodeMaker<NodeSuffixInitializerList> {

  NodeSuffixInitializerList() {
    precedence = 2;
    assoc = 2;
  }

  using pattern =
  DelimitedList<
    Atom<'{'>,
    Seq<
      Opt<
        Seq<NodeDesignation, Atom<'='>>,
        Seq<NodeIdentifier,  Atom<':'>> // This isn't in the C grammar but compndlit-1.c uses it?
      >,
      NodeInitializer
    >,
    Atom<','>,
    Atom<'}'>
  >;
};


struct NodeInitializer : public ParseNode, public NodeMaker<NodeInitializer> {
  using pattern = Oneof<
    NodeInitializerList,
    NodeExpression
  >;
};

//------------------------------------------------------------------------------

struct NodeFunctionIdentifier : public ParseNode, public NodeMaker<NodeFunctionIdentifier> {
  using pattern = Seq<
    Any<NodeAttribute, NodePointer>,
    Oneof<
      NodeIdentifier,
      Seq<Atom<'('>, NodeFunctionIdentifier, Atom<')'>>
    >
  >;
};

//function-definition:
//    declaration-specifiers declarator declaration-listopt compound-statement

struct NodeFunctionDefinition : public ParseNode, public NodeMaker<NodeFunctionDefinition> {
  using pattern = Seq<
    Any<NodeModifier>,
    Opt<NodeSpecifier>,
    Any<NodeModifier>,
    NodeFunctionIdentifier,
    NodeParamList,
    Any<NodeModifier>,
    Opt<NodeAsmSuffix>,
    Opt<NodeKeyword<"const">>,
    // This is old-style declarations after param list
    Opt<Some<
      Seq<NodeDeclaration, Atom<';'>>
    >>,
    NodeStatementCompound
  >;
};

//------------------------------------------------------------------------------

struct NodeConstructor : public ParseNode, public NodeMaker<NodeConstructor> {
  using pattern = Seq<
    NodeClassType,
    NodeParamList,
    Oneof<
      Atom<';'>,
      NodeStatementCompound
    >
  >;
};

//------------------------------------------------------------------------------
// FIXME this is messy

struct NodeDeclaration : public ParseNode, public NodeMaker<NodeDeclaration> {
  using pattern = Seq<
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
};

//------------------------------------------------------------------------------

template<typename P>
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
  PushPopScope<
    DelimitedBlock<
      Atom<'{'>,
      NodeStatement,
      Atom<'}'>
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementFor : public ParseNode, public NodeMaker<NodeStatementFor> {
  using pattern = Seq<
    Keyword<"for">,
    Atom<'('>,
    // This is _not_ the same as
    // Opt<Oneof<e, x>>, Atom<';'>
    Oneof<
      Seq<comma_separated<NodeExpression>, Atom<';'>>,
      Seq<comma_separated<NodeDeclaration>, Atom<';'>>,
      Atom<';'>
    >,
    Opt<comma_separated<NodeExpression>>,
    Atom<';'>,
    Opt<comma_separated<NodeExpression>>,
    Atom<')'>,
    Oneof<
      NodeStatementCompound,
      NodeStatement
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementElse : public ParseNode, public NodeMaker<NodeStatementElse> {
  using pattern =
  Seq<
    Keyword<"else">,
    NodeStatement
  >;
};

struct NodeStatementIf : public ParseNode, public NodeMaker<NodeStatementIf> {
  using pattern = Seq<
    Keyword<"if">,

    DelimitedList<
      Atom<'('>,
      NodeExpression,
      Atom<','>,
      Atom<')'>
    >,

    NodeStatement,
    Opt<NodeStatementElse>
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementReturn : public ParseNode, public NodeMaker<NodeStatementReturn> {
  using pattern = Seq<
    Keyword<"return">,
    Opt<NodeExpression>,
    Atom<';'>
  >;
};


//------------------------------------------------------------------------------

struct NodeStatementCase : public ParseNode, public NodeMaker<NodeStatementCase> {
  using pattern = Seq<
    Keyword<"case">,
    NodeExpression,
    Opt<Seq<
      // case 1...2: - this is supported by GCC?
      NodeEllipsis,
      NodeExpression
    >>,
    Atom<':'>,
    Any<Seq<
      Not<Keyword<"case">>,
      Not<Keyword<"default">>,
      NodeStatement
    >>
  >;
};

struct NodeStatementDefault : public ParseNode, public NodeMaker<NodeStatementDefault> {
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

struct NodeStatementSwitch : public ParseNode, public NodeMaker<NodeStatementSwitch> {
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

struct NodeStatementWhile : public ParseNode, public NodeMaker<NodeStatementWhile> {
  using pattern = Seq<
    Keyword<"while">,
    DelimitedList<
      Atom<'('>,
      NodeExpression,
      Atom<','>,
      Atom<')'>
    >,
    NodeStatement
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementDoWhile : public ParseNode, public NodeMaker<NodeStatementDoWhile> {
  using pattern = Seq<
    Keyword<"do">,
    NodeStatement,
    Keyword<"while">,
    DelimitedList<
      Atom<'('>,
      NodeExpression,
      Atom<','>,
      Atom<')'>
    >,
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementLabel : public ParseNode, public NodeMaker<NodeStatementLabel> {
  using pattern = Seq<
    NodeIdentifier,
    Atom<':'>,
    Opt<Atom<';'>>
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementBreak : public ParseNode, public NodeMaker<NodeStatementBreak> {
  using pattern = Seq<
    Keyword<"break">,
    Atom<';'>
  >;
};

struct NodeStatementContinue : public ParseNode, public NodeMaker<NodeStatementContinue> {
  using pattern = Seq<
    Keyword<"continue">,
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

struct NodeAsmRef : public ParseNode, public NodeMaker<NodeAsmRef> {
  using pattern = Seq<
    NodeAtom<LEX_STRING>,
    Opt<Seq<
      Atom<'('>,
      NodeExpression,
      Atom<')'>
    >>
  >;
};

struct NodeAsmRefs : public ParseNode, public NodeMaker<NodeAsmRefs> {
  using pattern = comma_separated<NodeAsmRef>;
};

//------------------------------------------------------------------------------

struct NodeAsmQualifiers : public ParseNode, public NodeMaker<NodeAsmQualifiers> {
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

struct NodeStatementAsm : public ParseNode, public NodeMaker<NodeStatementAsm> {
  using pattern = Seq<
    Oneof<
      Keyword<"asm">,
      Keyword<"__asm">,
      Keyword<"__asm__">
    >,
    Opt<NodeAsmQualifiers>,
    Atom<'('>,
    NodeAtom<LEX_STRING>, // assembly code
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
};

//------------------------------------------------------------------------------

struct NodeTypedef : public ParseNode, public NodeMaker<NodeTypedef> {
  using pattern = Seq<
    Opt<Keyword<"__extension__">>,
    Keyword<"typedef">,
    Oneof<
      NodeStruct,
      NodeUnion,
      NodeClass,
      NodeEnum,
      NodeDeclaration
    >
  >;

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
  using pattern = Seq<
    Keyword<"goto">,
    NodeExpression,
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

struct NodeStatement : public PatternWrapper<NodeStatement> {
  using pattern = Oneof<
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
    Seq<comma_separated<NodeExpression>,  Atom<';'>>,
    Seq<NodeDeclaration,                  Atom<';'>>,

    // Extra semicolons
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

struct NodeTranslationUnit : public ParseNode, public NodeMaker<NodeTranslationUnit> {
  using pattern = Any<
    Oneof<
      Seq<NodeClass,    Atom<';'>>,
      Seq<NodeStruct,   Atom<';'>>,
      Seq<NodeUnion,    Atom<';'>>,
      Seq<NodeEnum,     Atom<';'>>,
      NodeTypedef,

      NodePreproc,
      Seq<NodeTemplate, Atom<';'>>,
      NodeFunctionDefinition,
      Seq<NodeDeclaration, Atom<';'>>,
      Atom<';'>
    >
  >;
};

//------------------------------------------------------------------------------
#endif
