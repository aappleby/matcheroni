#pragma once

#include "ParseNode.h"
#include "c_constants.h"
#include <algorithm>

void dump_tree(const ParseNode* n, int max_depth, int indentation);

struct NodeClassType;
struct NodeEnumType;
struct NodeStructType;
struct NodeTypedefType;
struct NodeUnionType;

struct NodeSuffixInitializerList;
struct NodeAbstractDeclarator;
struct NodeClass;
struct NodeConstructor;
struct NodeDeclaration;
struct NodeDeclarator;
struct NodeEnum;
struct NodeExpression;
struct NodeFunctionDefinition;
struct NodeInitializer;
struct NodeInitializerList;
struct NodeSpecifier;
struct NodeStatement;
struct NodeStatementCompound;
struct NodeTypedef;
struct NodeStruct;
struct NodeTemplate;
struct NodeTypeDecl;
struct NodeTypeName;
struct NodeUnion;

typedef std::vector<const Token*> token_list;

//------------------------------------------------------------------------------

struct TypeScope {

  void clear() {
    class_types.clear();
    struct_types.clear();
    union_types.clear();
    enum_types.clear();
    typedef_types.clear();
  }

  bool has_type(token_list& types, Token* a) {
    if(atom_cmp(*a, LEX_IDENTIFIER)) return false;

    for (const auto c : types) {
      if (a->atom_cmp(*c) == 0) return true;
    }

    return false;
  }

  void add_type(token_list& types, Token* a) {
    DCHECK(atom_cmp(*a, LEX_IDENTIFIER) == 0);

    for (const auto& c : types) {
      if (a->atom_cmp(*c) == 0) return;
    }

    types.push_back(a);
  }

  //----------------------------------------

  bool has_class_type  (Token* a) { if (has_type(class_types,   a)) return true; if (parent) return parent->has_class_type  (a); else return false; }
  bool has_struct_type (Token* a) { if (has_type(struct_types,  a)) return true; if (parent) return parent->has_struct_type (a); else return false; }
  bool has_union_type  (Token* a) { if (has_type(union_types,   a)) return true; if (parent) return parent->has_union_type  (a); else return false; }
  bool has_enum_type   (Token* a) { if (has_type(enum_types,    a)) return true; if (parent) return parent->has_enum_type   (a); else return false; }
  bool has_typedef_type(Token* a) { if (has_type(typedef_types, a)) return true; if (parent) return parent->has_typedef_type(a); else return false; }

  void add_class_type  (Token* a) { return add_type(class_types,   a); }
  void add_struct_type (Token* a) { return add_type(struct_types,  a); }
  void add_union_type  (Token* a) { return add_type(union_types,   a); }
  void add_enum_type   (Token* a) { return add_type(enum_types,    a); }
  void add_typedef_type(Token* a) { return add_type(typedef_types, a); }

  TypeScope* parent;
  token_list class_types;
  token_list struct_types;
  token_list union_types;
  token_list enum_types;
  token_list typedef_types;
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

  Token* match_class_type  (Token* a, Token* b) { return type_scope->has_class_type  (a) ? a + 1 : nullptr; }
  Token* match_struct_type (Token* a, Token* b) { return type_scope->has_struct_type (a) ? a + 1 : nullptr; }
  Token* match_union_type  (Token* a, Token* b) { return type_scope->has_union_type  (a) ? a + 1 : nullptr; }
  Token* match_enum_type   (Token* a, Token* b) { return type_scope->has_enum_type   (a) ? a + 1 : nullptr; }
  Token* match_typedef_type(Token* a, Token* b) { return type_scope->has_typedef_type(a) ? a + 1 : nullptr; }

  void add_class_type  (Token* a) { type_scope->add_class_type  (a); }
  void add_struct_type (Token* a) { type_scope->add_struct_type (a); }
  void add_union_type  (Token* a) { type_scope->add_union_type  (a); }
  void add_enum_type   (Token* a) { type_scope->add_enum_type   (a); }
  void add_typedef_type(Token* a) { type_scope->add_typedef_type(a); }

  //----------------------------------------------------------------------------

  void push_scope() {
    TypeScope* new_scope = new TypeScope();
    new_scope->parent = type_scope;
    type_scope = new_scope;
  }

  void pop_scope() {
    TypeScope* old_scope = type_scope->parent;
    if (old_scope) {
      delete type_scope;
      type_scope = old_scope;
    }
  }

  //----------------------------------------------------------------------------

  void append_node(ParseNode* node) {
    if (tail) {
      tail->next = node;
      node->prev = tail;
      tail = node;
    }
    else {
      head = node;
      tail = node;
    }
  }

  //----------------------------------------------------------------------------

  void dump_stats();
  void dump_lexemes();
  void dump_tokens();

  std::string text;
  std::vector<Lexeme> lexemes;
  std::vector<Token>  tokens;

  ParseNode* head = nullptr;
  ParseNode* tail = nullptr;

  int file_pass = 0;
  int file_fail = 0;
  int file_skip = 0;
  int file_bytes = 0;
  int file_lines = 0;

  double io_accum = 0;
  double lex_accum = 0;
  double parse_accum = 0;
  double cleanup_accum = 0;

  TypeScope* type_scope;
};

//------------------------------------------------------------------------------

template<auto P>
struct NodeAtom : public NodeMaker<NodeAtom<P>> {
  using pattern = Atom<P>;
};

template<StringParam lit>
struct NodeKeyword : public NodeMaker<NodeKeyword<lit>> {
  using pattern = Keyword<lit>;
};

template<StringParam lit>
struct NodeLiteral : public NodeMaker<NodeLiteral<lit>> {
  using pattern = Literal2<lit>;
};

//------------------------------------------------------------------------------
// Our builtin types are any sequence of prefixes followed by a builtin type

struct NodeBuiltinType : public NodeMaker<NodeBuiltinType> {
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

struct NodeTypedefType : public NodeMaker<NodeTypedefType> {
  using pattern = Ref<&C99Parser::match_typedef_type>;
};


//------------------------------------------------------------------------------
// Excluding builtins and typedefs from being identifiers changes the total
// number of parse nodes, but why?

// - Because "uint8_t *x = 5" gets misparsed as an expression if uint8_t matches
// as an identifier

struct NodeIdentifier : public NodeMaker<NodeIdentifier> {
  using pattern =
  Seq<
    Not<NodeBuiltinType>,
    Not<NodeTypedefType>,
    Atom<LEX_IDENTIFIER>
  >;
};

//------------------------------------------------------------------------------

struct NodePreproc : public NodeMaker<NodePreproc> {
  using pattern = Atom<LEX_PREPROC>;
};

//------------------------------------------------------------------------------

struct NodeConstant : public NodeMaker<NodeConstant> {
  using pattern = Oneof<
    Atom<LEX_FLOAT>,
    Atom<LEX_INT>,
    Atom<LEX_CHAR>,
    Some<Atom<LEX_STRING>>
  >;
};

template<StringParam lit>
struct NodeOperator : public ParseNode {
  static Token* match(void* ctx, Token* a, Token* b) {
    /*
    for (auto c = a; c < b; c++) {
      c->clear_span();
    }
    */
    auto end = match_punct(ctx, a, b, lit.str_val, lit.str_len);
    if (end && end != a) {
      auto node = new NodeOperator<lit>();
      node->init(a, end - 1);
    }
    return end;
  }
};

//------------------------------------------------------------------------------

/*
template<typename NodeType, typename pattern>
struct Capture {
  static Token* match(void* ctx, Token* a, Token* b) {
    if (!a || a == b) return nullptr;
    auto end = pattern::match(ctx, a, b);
    if (end && end != a) {
      auto node = new NodeType();
      node->init(a, end - 1);
    }
    return end;
  }
};
*/

template<typename P>
using comma_separated = Seq<P, Any<Seq<Atom<','>, P>>, Opt<Atom<','>> >;

template<typename P>
using opt_comma_separated = Opt<comma_separated<P>>;

//------------------------------------------------------------------------------












//------------------------------------------------------------------------------

struct NodeOpPrefix : public ParseNode {};

template<StringParam lit>
struct MatchOpPrefix {
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = match_punct(ctx, a, b, lit.str_val, lit.str_len);
    if (end) {
      auto node = new NodeOpPrefix();
      node->precedence = prefix_precedence(lit.str_val);
      node->assoc      = prefix_assoc(lit.str_val);
      node->init(a, end - 1);
    }
    return end;
  }
};

//------------------------------------------------------------------------------

struct NodeOpBinary : public ParseNode {};

template<StringParam lit>
struct MatchOpBinary {
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = match_punct(ctx, a, b, lit.str_val, lit.str_len);
    if (end) {
      auto node = new NodeOpBinary();
      node->precedence = binary_precedence(lit.str_val);
      node->assoc      = binary_assoc(lit.str_val);
      node->init(a, end - 1);
    }
    return end;
  }
};

//------------------------------------------------------------------------------

struct NodeOpSuffix : public ParseNode {};

template<StringParam lit>
struct MatchOpSuffix {
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = match_punct(ctx, a, b, lit.str_val, lit.str_len);
    if (end) {
      auto node = new NodeOpSuffix();
      node->precedence = suffix_precedence(lit.str_val);
      node->assoc      = suffix_assoc(lit.str_val);
      node->init(a, end - 1);
    }
    return end;
  }
};

//------------------------------------------------------------------------------

struct NodeQualifier : public NodeMaker<NodeQualifier> {
  static Token* match(void* ctx, Token* a, Token* b) {
    if (!a || a == b) return nullptr;
    if (SST<qualifiers>::contains(*a)) {
      auto node = new NodeQualifier();
      node->init(a, a);
      return a + 1;
    }
    else {
      return nullptr;
    }
  }
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
    Some<NodeAtom<LEX_STRING>>,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeAccessSpecifier : public NodeMaker<NodeAccessSpecifier> {
  using pattern = Seq<
    Oneof<
      NodeLiteral<"public">,
      NodeLiteral<"private">
    >,
    Atom<':'>
  >;
};






























































//------------------------------------------------------------------------------
// (6.5.4) cast-expression:
//   unary-expression
//   ( type-name ) cast-expression

struct NodeParenType : public NodeMaker<NodeParenType> {
  using pattern = Seq<
    Atom<'('>,
    NodeTypeName,
    Atom<')'>
  >;
};

struct NodePrefixCast : public NodeMaker<NodePrefixCast> {
  virtual void init(Token* tok_a, Token* tok_b) override {
    NodeMaker::init(tok_a, tok_b);
    this->precedence = 3;
    this->assoc = -2;
  }

  using pattern = Seq<
    Atom<'('>,
    NodeTypeName,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeExpressionParen : public NodeMaker<NodeExpressionParen> {
  using pattern =
  DelimitedList<
    Atom<'('>,
    NodeExpression,
    Atom<','>,
    Atom<')'>
  >;
};

struct NodeSuffixParen : public NodeMaker<NodeSuffixParen> {
  virtual void init(Token* tok_a, Token* tok_b) override {
    NodeMaker::init(tok_a, tok_b);
    this->precedence = 2;
    this->assoc = 2;
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

struct NodeExpressionBraces : public NodeMaker<NodeExpressionBraces> {
  using pattern =
  DelimitedList<
    Atom<'{'>,
    NodeExpression,
    Atom<','>,
    Atom<'}'>
  >;
};

struct NodeSuffixBraces : public NodeMaker<NodeSuffixBraces> {
  virtual void init(Token* tok_a, Token* tok_b) override {
    NodeMaker::init(tok_a, tok_b);
    this->precedence = 2;
    this->assoc = 2;
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

struct NodeSuffixSubscript : public NodeMaker<NodeSuffixSubscript> {
  virtual void init(Token* tok_a, Token* tok_b) override {
    NodeMaker::init(tok_a, tok_b);
    this->precedence = 2;
    this->assoc = 2;
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

struct NodeExpressionGccCompound : public NodeMaker<NodeExpressionGccCompound> {
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

struct NodeExpressionSizeof  : public NodeMaker<NodeExpressionSizeof> {
  using pattern = Seq<
    Keyword<"sizeof">,
    Oneof<
      NodeParenType,
      NodeExpressionParen,
      NodeExpression
    >
  >;
};

struct NodeExpressionAlignof  : public NodeMaker<NodeExpressionAlignof> {
  using pattern = Seq<
    Keyword<"__alignof__">,
    Oneof<
      NodeParenType,
      NodeExpressionParen
    >
  >;
};

struct NodeExpressionOffsetof  : public NodeMaker<NodeExpressionOffsetof> {
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
struct NodePrefixKeyword : public NodeMaker<NodePrefixKeyword<lit>> {
  virtual void init(Token* tok_a, Token* tok_b) override {
    NodeMaker<NodePrefixKeyword<lit>>::init(tok_a, tok_b);
    this->precedence = 3;
    this->assoc = -2;
  }

  //using pattern = Keyword<lit>;
  static Token* match(void* ctx, Token* a, Token* b) {
    if (!a || a == b) return nullptr;

    print_trace_start<NodePrefixKeyword<lit>, Token>(a);
    Token* end = Keyword<lit>::match(ctx, a, b);
    if (end) {
      auto node = new NodePrefixKeyword<lit>();
      node->init(a, a);
    }
    print_trace_end<NodePrefixKeyword<lit>, Token>(a, end);
    return end;
  }
};

using ExpressionPrefixOp =
Oneof<
  NodePrefixCast,
  NodePrefixKeyword<"__extension__">,
  NodePrefixKeyword<"__real">,
  NodePrefixKeyword<"__real__">,
  NodePrefixKeyword<"__imag">,
  NodePrefixKeyword<"__imag__">,
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
  MatchOpSuffix<"++">,
  MatchOpSuffix<"--">
>;

//----------------------------------------

using unit_pattern = Seq<
  Any<ExpressionPrefixOp>,
  ExpressionCore,
  Any<ExpressionSuffixOp>
>;

/*
struct NodeExpressionUnit : public NodeMaker<NodeExpressionUnit> {
  using pattern = Seq<
    Any<ExpressionPrefixOp>,
    ExpressionCore,
    Any<ExpressionSuffixOp>
  >;
};
*/

/*
using SpanTernaryOp = // Not covered by csmith
Seq<
  // pr68249.c - ternary option can be empty
  // pr49474.c - ternary branches can be comma-lists
  MatchOpBinary<"?">,
  Opt<comma_separated<unit_pattern>>,
  MatchOpBinary<":">,
  Opt<comma_separated<unit_pattern>>
>;
*/

struct NodeTernaryOp : public NodeMaker<NodeTernaryOp> {
  virtual void init(Token* tok_a, Token* tok_b) override {
    NodeMaker<NodeTernaryOp>::init(tok_a, tok_b);
    this->precedence = 16;
    this->assoc = -1;
  }

  using pattern =
  Seq<
    MatchOpBinary<"?">,
    Opt<comma_separated<NodeExpression>>,
    MatchOpBinary<":">
  >;
};


//----------------------------------------

struct NodeExpression : public ParseNode {

  static Token* match_binary_op(void* ctx, Token* a, Token* b) {
    if (!a || a == b) return nullptr;

    if (atom_cmp(*a, LEX_PUNCT)) return nullptr;

    switch(a->unsafe_span_a()[0]) {
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
      case '?': return NodeTernaryOp::match(ctx, a, b);

      // FIXME this is only for C++, and
      //case ':': return MatchOpBinary<"::">::match(ctx, a, b);
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
    if (tok_b == nullptr) return nullptr;


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
          node->init(l->get_span(), c->get_span());
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
      node->init(a, end - 1);
    }
    print_trace_end<NodeExpression, Token>(a, end);
    return end;
  }

};

//------------------------------------------------------------------------------
// 20010911-1.c - Attribute can be empty

struct NodeAttribute : public NodeMaker<NodeAttribute> {
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

struct NodeAlignas : public NodeMaker<NodeAlignas> {
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

struct NodeDeclspec : public NodeMaker<NodeDeclspec> {
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

struct NodeTypeDecl : public NodeMaker<NodeTypeDecl> {
  using pattern = Seq<
    Any<NodeModifier>,
    NodeSpecifier,
    Opt<NodeAbstractDeclarator>
  >;
};

//------------------------------------------------------------------------------

struct NodePointer : public NodeMaker<NodePointer> {
  using pattern =
  Seq<
    MatchOpPrefix<"*">,
    Any<
      MatchOpPrefix<"*">,
      NodeModifier
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeParam : public NodeMaker<NodeParam> {
  using pattern = Oneof<
    NodeOperator<"...">,
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

struct NodeParamList : public NodeMaker<NodeParamList> {
  using pattern =
  DelimitedList<
    Atom<'('>,
    NodeParam,
    Atom<','>,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeArraySuffix : public NodeMaker<NodeArraySuffix> {
  using pattern = Oneof<
    Seq<Atom<'['>,                         Any<NodeModifier>,                           Opt<NodeExpression>, Atom<']'>>,
    Seq<Atom<'['>, NodeKeyword<"static">,  Any<NodeModifier>,                               NodeExpression,  Atom<']'>>,
    Seq<Atom<'['>,                         Any<NodeModifier>,   NodeKeyword<"static">,      NodeExpression,  Atom<']'>>,
    Seq<Atom<'['>,                         Any<NodeModifier>,   NodeOperator<"*">,                           Atom<']'>>
  >;
};

//------------------------------------------------------------------------------

struct NodeTemplateArgs : public NodeMaker<NodeTemplateArgs> {
  using pattern =
  DelimitedList<
    Atom<'<'>,
    NodeExpression,
    Atom<','>,
    Atom<'>'>
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

struct NodeTypeName : public NodeMaker<NodeTypeName> {
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

struct NodeBitSuffix : public NodeMaker<NodeBitSuffix> {
  using pattern = Seq< Atom<':'>, NodeExpression >;
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

struct NodeDeclaratorList : public NodeMaker<NodeDeclaratorList> {
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

struct NodeFieldList : public NodeMaker<NodeFieldList> {
  using pattern =
  DelimitedBlock<
    Atom<'{'>,
    NodeField,
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

struct NodeStructType : public NodeMaker<NodeStructType> {
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

struct NodeStruct : public NodeMaker<NodeStruct> {
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
      node->init(a, end - 1);
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

struct NodeUnion : public NodeMaker<NodeUnion> {
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

struct NodeClassType : public NodeMaker<NodeClassType> {
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

struct NodeClass : public NodeMaker<NodeClass> {
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

struct NodeTemplateParams : public NodeMaker<NodeTemplateParams> {
  using pattern =
  DelimitedList<
    Atom<'<'>,
    NodeDeclaration,
    Atom<','>,
    Atom<'>'>
  >;
};

struct NodeTemplate : public NodeMaker<NodeTemplate> {
  using pattern = Seq<
    NodeLiteral<"template">,
    NodeTemplateParams,
    NodeClass
  >;
};

//------------------------------------------------------------------------------
// FIXME should probably have a few diffeerent versions instead of all the opts

struct NodeEnumType : public NodeMaker<NodeEnumType> {
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

struct NodeEnumerator : public NodeMaker<NodeEnumerator> {
  using pattern = Seq<
    NodeIdentifier,
    Opt<Seq<Atom<'='>, NodeExpression>>
  >;
};

struct NodeEnumerators : public NodeMaker<NodeEnumerators> {
  using pattern =
  DelimitedList<
    Atom<'{'>,
    NodeEnumerator,
    Atom<','>,
    Atom<'}'>
  >;
};

struct NodeEnum : public NodeMaker<NodeEnum> {
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

struct NodeDesignation : public NodeMaker<NodeDesignation> {
  using pattern =
  Some<
    Seq<Atom<'['>, NodeConstant,   Atom<']'>>,
    Seq<Atom<'['>, NodeIdentifier, Atom<']'>>,
    Seq<Atom<'.'>, NodeIdentifier>
  >;
};

struct NodeInitializerList : public NodeMaker<NodeInitializerList> {
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

struct NodeSuffixInitializerList : public NodeMaker<NodeSuffixInitializerList> {

  virtual void init(Token* tok_a, Token* tok_b) override {
    NodeMaker::init(tok_a, tok_b);
    this->precedence = 2;
    this->assoc = 2;
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


struct NodeInitializer : public NodeMaker<NodeInitializer> {
  using pattern = Oneof<
    NodeInitializerList,
    NodeExpression
  >;
};

//------------------------------------------------------------------------------

struct NodeFunctionIdentifier : public NodeMaker<NodeFunctionIdentifier> {
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

struct NodeFunctionDefinition : public NodeMaker<NodeFunctionDefinition> {
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

struct NodeConstructor : public NodeMaker<NodeConstructor> {
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

struct NodeDeclaration : public NodeMaker<NodeDeclaration> {
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

struct NodeStatementCompound : public NodeMaker<NodeStatementCompound> {
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

struct NodeStatementFor : public NodeMaker<NodeStatementFor> {
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

struct NodeStatementReturn : public NodeMaker<NodeStatementReturn> {
  using pattern = Seq<
    Keyword<"return">,
    Opt<NodeExpression>,
    Atom<';'>
  >;
};


//------------------------------------------------------------------------------

struct NodeStatementCase : public NodeMaker<NodeStatementCase> {
  using pattern = Seq<
    Keyword<"case">,
    NodeExpression,
    Opt<Seq<
      // case 1...2: - this is supported by GCC?
      NodeOperator<"...">,
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

struct NodeStatementDoWhile : public NodeMaker<NodeStatementDoWhile> {
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

struct NodeStatementLabel : public NodeMaker<NodeStatementLabel> {
  using pattern = Seq<
    NodeIdentifier,
    Atom<':'>,
    Opt<Atom<';'>>
  >;
};

//------------------------------------------------------------------------------

struct NodeStatementBreak : public NodeMaker<NodeStatementBreak> {
  using pattern = Seq<
    Keyword<"break">,
    Atom<';'>
  >;
};

struct NodeStatementContinue : public NodeMaker<NodeStatementContinue> {
  using pattern = Seq<
    Keyword<"continue">,
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

struct NodeAsmRef : public NodeMaker<NodeAsmRef> {
  using pattern = Seq<
    NodeAtom<LEX_STRING>,
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

struct NodeTypedef : public NodeMaker<NodeTypedef> {
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

struct NodeStatementGoto : public NodeMaker<NodeStatementGoto> {
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

struct NodeTranslationUnit : public NodeMaker<NodeTranslationUnit> {
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
