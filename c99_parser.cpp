#include "Matcheroni.h"

#include "Lexemes.h"
#include "Node.h"
#include "NodeTypes.h"
#include "Tokens.h"

#include <filesystem>
#include <memory.h>

double timestamp_ms();

#ifdef MATCHERONI_USE_NAMESPACE
using namespace matcheroni;
#endif

//------------------------------------------------------------------------------

bool atom_eq(const Token& a, const TokenType& b) {
  return a.type == b;
}

bool atom_eq(const Token& a, const char& b) {
  return a.lex->len() == 1 && (*a.lex->span_a == b);
}

template<int N>
bool atom_eq(const Token& a, const StringParam<N>& b) {
  if (a.lex->len() != b.len) return false;
  for (auto i = 0; i < b.len; i++) {
    if (a.lex->span_a[i] != b.value[i]) return false;
  }

  return true;
}

//------------------------------------------------------------------------------

struct NodeStack {
  void push(NodeBase* n) {
    assert(_stack[_top] == nullptr);
    _stack[_top] = n;
    _top++;
  }

  NodeBase* pop() {
    assert(_top);
    _top--;
    NodeBase* result = _stack[_top];
    _stack[_top] = nullptr;
    return result;
  }

  size_t top() const { return _top; }

  void dump_top() {
    _stack[_top-1]->dump_tree();
  }

  void clear_to(size_t new_top) {
    while(_top > new_top) {
      delete pop();
    }
  }

  void pop_to(size_t new_top) {
    while (_top > new_top) pop();
  }

  NodeBase*  _stack[256] = {0};
  size_t _top = 0;
};

NodeStack node_stack;

//------------------------------------------------------------------------------

template<typename pattern>
struct Dump {
  static const Token* match(const Token* a, const Token* b) {
    auto end = pattern::match(a, b);
    if (end) node_stack.dump_top();
    return end;
  }
};

//----------------------------------------

const char* find_matching_delim(const char* a, const char* b) {
  char ldelim = *a++;

  char rdelim = 0;
  if (ldelim == '<')  rdelim = '>';
  if (ldelim == '{')  rdelim = '}';
  if (ldelim == '[')  rdelim = ']';
  if (ldelim == '"')  rdelim = '"';
  if (ldelim == '\'') rdelim = '\'';
  if (!rdelim) return nullptr;

  while(a && *a && a < b) {
    if (*a == rdelim) return a;

    if (*a == '<' || *a == '{' || *a == '[' || *a == '"' || *a == '\'') {
      a = find_matching_delim(a, b);
      if (!a) return nullptr;
      a++;
    }
    else if (ldelim == '"' && a[0] == '\\' && a[1] == '"') {
      a += 2;
    }
    else if (ldelim == '\'' && a[0] == '\\' && a[1] == '\'') {
      a += 2;
    }
    else {
      a++;
    }
  }

  return nullptr;
}

//----------------------------------------

const Token* find_matching_delim(char ldelim, char rdelim, const Token* a, const Token* b) {
  if (*a->lex->span_a != ldelim) return nullptr;
  a++;

  while(a && a < b) {
    if (a->is_punct(rdelim)) return a;

    // Note that we _don't_ recurse through <> because they're not guaranteed
    // to be delimiters. Annoying aspect of C. :/

    if (a && a->is_punct('(')) a = find_matching_delim('(', ')', a, b);
    if (a && a->is_punct('{')) a = find_matching_delim('{', '}', a, b);
    if (a && a->is_punct('[')) a = find_matching_delim('[', ']', a, b);

    if (!a) return nullptr;
    a++;
  }

  return nullptr;
}

//------------------------------------------------------------------------------
// The Delimited<> modifier constrains a matcher to fit exactly between a pair
// of matching delimiters.
// For example, Delimited<'(', ')', NodeConstant> will match "(1)" but not
// "(1 + 2)".

template<char ldelim, char rdelim, typename P>
struct Delimited {
  static const Token* match(const Token* a, const Token* b) {
    if (!a || !a->is_punct(ldelim)) return nullptr;
    auto new_b = find_matching_delim(ldelim, rdelim, a, b);
    if (!new_b || !new_b->is_punct(rdelim)) return nullptr;

    if (!new_b) return nullptr;
    if (auto end = P::match(a + 1, new_b)) {
      if (end != new_b) return nullptr;
      return new_b + 1;
    }
    else {
      return nullptr;
    }
  }
};

//------------------------------------------------------------------------------

template<typename P>
using comma_separated = Seq<P,Any<Seq<Atom<','>, P>>>;

template<typename P>
using opt_comma_separated = Opt<comma_separated<P>>;

//------------------------------------------------------------------------------

template<typename NT>
struct NodeMaker : public NodeBase {
  NodeMaker(const Token* a, const Token* b, NodeBase** children, size_t child_count)
  : NodeBase(NT::node_type, a, b, children, child_count) {
  }

  static const Token* match(const Token* a, const Token* b) {
    auto old_top = node_stack.top();
    auto end = NT::pattern::match(a, b);
    auto new_top = node_stack.top();

    if (end && end != a) {
      auto node = new NT(a, end, &node_stack._stack[old_top], new_top - old_top);
      node_stack.pop_to(old_top);
      node_stack.push(node);
      return end;
    }
    else {
      node_stack.clear_to(old_top);
      return nullptr;
    }
  }
};

//------------------------------------------------------------------------------
// Predecls

const Token* parse_statement(const Token* a, const Token* b);
const Token* parse_expression(const Token* a, const Token* b);

//------------------------------------------------------------------------------

struct NodeIdentifier : public NodeMaker<NodeIdentifier> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_IDENTIFIER;

  using pattern = Atom<TOK_IDENTIFIER>;
};

//------------------------------------------------------------------------------

struct NodeConstant : public NodeMaker<NodeConstant> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_CONSTANT;

  using pattern = Oneof<
    Atom<TOK_FLOAT>,
    Atom<TOK_INT>,
    Atom<TOK_CHAR>,
    Atom<TOK_STRING>
  >;
};

//------------------------------------------------------------------------------

struct NodeSpecifier : public NodeMaker<NodeSpecifier> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_SPECIFIER;

  using pattern = Oneof<
    Keyword<"extern">,
    Keyword<"static">,
    Keyword<"register">,
    Keyword<"inline">,
    Keyword<"thread_local">,
    Keyword<"const">,
    Keyword<"volatile">,
    Keyword<"restrict">,
    Keyword<"__restrict__">,
    Keyword<"_Atomic">,
    Keyword<"_Noreturn">,
    Keyword<"mutable">,
    Keyword<"constexpr">,
    Keyword<"constinit">,
    Keyword<"consteval">,
    Keyword<"virtual">,
    Keyword<"explicit">
  >;
};

//------------------------------------------------------------------------------

struct NodeSpecifierList : public NodeMaker<NodeSpecifierList> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_LIST_SPECIFIER;

  using pattern = Some<NodeSpecifier>;
};

//------------------------------------------------------------------------------

struct NodeCompoundStatement : public NodeMaker<NodeCompoundStatement> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_STATEMENT_COMPOUND;

  using pattern = Seq<
    Atom<'{'>,
    Any<Ref<parse_statement>>,
    Atom<'}'>
  >;
};

//------------------------------------------------------------------------------


struct NodeTemplateArgs : public NodeMaker<NodeTemplateArgs> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_LIST_ARGUMENT;

  using pattern = Delimited<'<', '>',
    opt_comma_separated<Ref<parse_expression>>
  >;
};

//------------------------------------------------------------------------------

struct NodeDecltype : public NodeMaker<NodeDecltype> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_DECLTYPE;

  using pattern = Seq<
    Opt<NodeSpecifierList>,
    Opt<Keyword<"struct">>,
    NodeIdentifier,
    Opt<NodeSpecifierList>,
    Opt<NodeTemplateArgs>,
    Opt<Seq<
      Atom<':'>,
      Atom<':'>,
      NodeIdentifier
    >>,
    Opt<Atom<'*'>>
  >;
};

//------------------------------------------------------------------------------
// FIXME should probably have a few diffeerent versions instead of all the opts

struct NodeEnum : public NodeMaker<NodeEnum> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_DECLARATION_ENUM;

  using pattern = Seq<
    Keyword<"enum">,
    Opt<Keyword<"class">>,
    Opt<NodeIdentifier>,
    Opt<Seq<Atom<':'>, NodeDecltype>>,
    Opt<Seq<
      Atom<'{'>,
      comma_separated<Ref<parse_expression>>,
      Atom<'}'>
    >>,
    Opt<NodeIdentifier>,
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

struct NodeArraySuffix : public NodeMaker<NodeArraySuffix> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_ARRAY_SUFFIX;

  using pattern = Seq<
    Atom<'['>,
    Ref<parse_expression>,
    Atom<']'>
  >;
};

//------------------------------------------------------------------------------

struct NodeBitsizeSuffix : public NodeMaker<NodeBitsizeSuffix> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_BITSIZE;

  using pattern = Seq<
    Atom<':'>,
    Ref<parse_expression>
  >;
};

//------------------------------------------------------------------------------

struct NodeDeclarationAtom : public NodeMaker<NodeDeclarationAtom> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_DECLARATION_ATOM;

  using pattern = Seq<
    NodeIdentifier,
    Opt<NodeBitsizeSuffix>,
    Any<NodeArraySuffix>,
    Opt<Seq<
      Atom<'='>,
      Ref<parse_expression>
    >>
  >;
};

//------------------------------------------------------------------------------

struct NodeDeclaration : public NodeMaker<NodeDeclaration> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_DECLARATION;

  using pattern = Seq<
    NodeDecltype,
    Oneof<
      comma_separated<NodeDeclarationAtom>,
      Some<NodeArraySuffix>,
      Nothing
    >
  >;

  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<NodeDeclaration>::match(a, b);
    return end;
  }
};

//------------------------------------------------------------------------------

struct NodeDeclarationStatement : public NodeMaker<NodeDeclarationStatement> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_STATEMENT_DECL;

  using pattern = Seq<
    NodeDeclaration,
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

struct NodeDeclList : public NodeMaker<NodeDeclList> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_LIST_PARAMETER;

  using pattern = Seq<
    Atom<'('>,
    opt_comma_separated<NodeDeclaration>,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeInitializerList : public NodeMaker<NodeInitializerList> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_LIST_INITIALIZER;

  using pattern = Seq<
    Atom<':'>,
    comma_separated<Ref<parse_expression>>,
    And<Atom<'{'>>
  >;
};

struct NodeConstructor : public NodeMaker<NodeConstructor> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_CONSTRUCTOR;

  using pattern = Seq<
    NodeIdentifier,
    NodeDeclList,
    Opt<NodeInitializerList>,
    Oneof<
      NodeCompoundStatement,
      Atom<';'>
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeFunction : public NodeMaker<NodeFunction> {
  //using NodeMaker::NodeMaker;
  NodeFunction(const Token* a, const Token* b, NodeBase** children, size_t child_count)
  : NodeMaker(a, b, children, child_count) {
    _type = children[0]->as<NodeDecltype>();
    _name = children[1]->as<NodeIdentifier>();
    _args = children[2]->as<NodeDeclList>();
    _body = children[3]->as<NodeCompoundStatement>();
  }

  NodeDecltype*   _type = nullptr;
  NodeIdentifier* _name = nullptr;
  NodeDeclList*   _args = nullptr;
  NodeCompoundStatement* _body = nullptr;

  static constexpr NodeType node_type = NODE_DEFINITION_FUNCTION;

  using pattern = Seq<
    NodeDecltype,
    NodeIdentifier,
    NodeDeclList,
    Opt<Keyword<"const">>,
    Oneof<
      NodeCompoundStatement,
      Atom<';'>
    >
  >;
};

//------------------------------------------------------------------------------

struct NodeAccessSpecifier : public NodeMaker<NodeAccessSpecifier> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_ACCESS_SPECIFIER;

  using pattern = Seq<
    Oneof<
      Keyword<"public">,
      Keyword<"private">
    >,
    Atom<':'>
  >;
};

//------------------------------------------------------------------------------

struct NodeFieldList : public NodeMaker<NodeFieldList> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_LIST_FIELD;

  using pattern = Seq<
    Atom<'{'>,
    Any<Oneof<
      NodeAccessSpecifier,
      NodeConstructor,
      NodeEnum,
      NodeFunction,
      NodeDeclarationStatement
    >>,
    Atom<'}'>
  >;
};

//------------------------------------------------------------------------------

struct NodeClass : public NodeMaker<NodeClass> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_DECLARATION_CLASS;

  using pattern = Seq<
    Keyword<"class">,
    NodeIdentifier,
    Opt<NodeFieldList>,
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

struct NodeStruct : public NodeMaker<NodeStruct> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_DECLARATION_STRUCT;

  using pattern = Seq<
    Keyword<"struct">,
    NodeIdentifier,
    Opt<NodeFieldList>,
    Atom<';'>
  >;
};

//----------------------------------------

struct NodeNamespace : public NodeMaker<NodeNamespace> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_DECLARATION_NAMESPACE;

  using pattern = Seq<
    Keyword<"namespace">,
    NodeIdentifier,
    Opt<NodeFieldList>,
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

struct NodeTemplateParams : public NodeMaker<NodeTemplateParams> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_LIST_TEMPLATE_PARAMETER;

  using pattern = Delimited<'<', '>', comma_separated<NodeDeclaration>>;
};





















































//==============================================================================
// EXPRESSION STUFF

// LeftAssocSuffix  ((a++)++)
// RightAssocPrefix (++(++a))
// LeftAssocInfix   ((a-b)-c)
// RightAssocInfix  (a=(b=c))

template<typename N, typename P, typename O>
struct LeftAssocInfix {
  static const Token* match(const Token* a, const Token* b) {
    auto old_top = node_stack.top();

    auto end = P::match(a, b);
    if (!end) {
      // No node match, can't do anything.
      return nullptr;
    }

    while(1) {
      end = O::match(end, b);
      if (!a) {
        // Node match but no op, return the match.
        return end;
      }
      end = P::match(end, b);
      if (!end) {
        // Left node and op match, but no right node match.
        // Can't proceed, clear the failed match and return.
        node_stack.clear_to(old_top);
        return nullptr;
      }

      // Matched P-O-P, group them together into a N and continue matching.
      auto new_top = node_stack.top();
      assert((new_top - old_top) == 3);
      auto node = new N(a, end, &node_stack._stack[old_top], new_top - old_top);
      node_stack.pop_to(old_top);
      node_stack.push(node);
      a = end;
    }
  }
};

//------------------------------------------------------------------------------
// Matches strings of punctuation

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
struct NodeOperator : public NodeBase {
  NodeOperator(const Token* a, const Token* b)
  : NodeBase(NODE_OPERATOR, a, b) {
  }

  static const Token* match(const Token* a, const Token* b) {
    if (auto end = Operator<lit>::match(a, b)) {
      node_stack.push(new NodeOperator(a, end));
      return end;
    }
    else {
      return nullptr;
    }
  }
};

//------------------------------------------------------------------------------
// All

/* 0  */ const Token* parse_expression_paren       (const Token* a, const Token* b);
/* 1  */ const Token* parse_expression_scope       (const Token* a, const Token* b);

/* 2  */ const Token* parse_expression_postincdec  (const Token* a, const Token* b);
/* 2  */ const Token* parse_expression_fcast       (const Token* a, const Token* b);
/* 2  */ const Token* parse_expression_call        (const Token* a, const Token* b);
/* 2  */ const Token* parse_expression_subscript   (const Token* a, const Token* b);
/* 2  */ const Token* parse_expression_member      (const Token* a, const Token* b);

/* 3  */ const Token* parse_expression_preincdec   (const Token* a, const Token* b);
/* 3  */ const Token* parse_expression_plusminus   (const Token* a, const Token* b);
/* 3  */ const Token* parse_expression_not         (const Token* a, const Token* b);
/* 3  */ const Token* parse_expression_cast        (const Token* a, const Token* b);
/* 3  */ const Token* parse_expression_dereference (const Token* a, const Token* b);
/* 3  */ const Token* parse_expression_addressof   (const Token* a, const Token* b);
/* 3  */ const Token* parse_expression_sizeof      (const Token* a, const Token* b);

/* 4  */ const Token* parse_expression_ptrtomember (const Token* a, const Token* b);
/* 5  */ const Token* parse_expression_muldivmod   (const Token* a, const Token* b);
/* 6  */ const Token* parse_expression_addsub      (const Token* a, const Token* b);
/* 7  */ const Token* parse_expression_shift       (const Token* a, const Token* b);
/* 8  */ const Token* parse_expression_threeway    (const Token* a, const Token* b);
/* 9  */ const Token* parse_expression_relational  (const Token* a, const Token* b);
/* 10 */ const Token* parse_expression_equality    (const Token* a, const Token* b);
/* 11 */ const Token* parse_expression_bitwise_and (const Token* a, const Token* b);
/* 12 */ const Token* parse_expression_bitwise_xor (const Token* a, const Token* b);
/* 13 */ const Token* parse_expression_bitwise_or  (const Token* a, const Token* b);
/* 14 */ const Token* parse_expression_logical_and (const Token* a, const Token* b);
/* 15 */ const Token* parse_expression_logical_or  (const Token* a, const Token* b);
/* 16 */ const Token* parse_expression_ternary     (const Token* a, const Token* b);
/* 16 */ const Token* parse_expression_assignment  (const Token* a, const Token* b);
/* 17 */ const Token* parse_expression_comma       (const Token* a, const Token* b);





//------------------------------------------------------------------------------

struct NodeExpressionParenthesized : public NodeMaker<NodeExpressionParenthesized> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_PAREN;

  using pattern = Seq<
    Atom<'('>,
    Ref<parse_expression>,
    Atom<')'>
  >;
};

const Token* parse_expression_paren(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionParenthesized,
    NodeConstant,
    NodeIdentifier
  >;
  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionScope : public NodeMaker<NodeExpressionScope> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_SCOPE;

  using pattern =
  Seq<
    Ref<parse_expression_paren>,
    Some<Seq<
      NodeOperator<"::">,
      Ref<parse_expression_paren>
    >>
  >;
};

const Token* parse_expression_scope(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionScope,
    Ref<parse_expression_paren>
  >;
  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionPostIncDec : public NodeMaker<NodeExpressionPostIncDec> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_POSTINCDEC;

  using pattern =
  Seq<
    Ref<parse_expression_scope>,
    Some<
      Oneof<
        NodeOperator<"++">,
        NodeOperator<"--">
      >
    >
  >;
};

const Token* parse_expression_postincdec(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionPostIncDec,
    Ref<parse_expression_scope>
  >;
  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionFCast : public NodeMaker<NodeExpressionFCast> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_FCAST;

  using pattern =
  Seq<
    NodeDecltype,
    Operator<"{">,
    Ref<parse_expression>,
    Operator<"}">
  >;
};

const Token* parse_expression_fcast(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionFCast,
    Ref<parse_expression_postincdec>
  >;
  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionCall : public NodeMaker<NodeExpressionCall> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_CALL;

  using pattern =
  Seq<
    Ref<parse_expression_fcast>,
    // FIXME does this need Some<Seq<...>>? Will f()() parse OK?
    Operator<"(">,
    Ref<parse_expression>,
    Operator<")">
  >;
};

const Token* parse_expression_call(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionCall,
    Ref<parse_expression_fcast>
  >;
  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionSubscript : public NodeMaker<NodeExpressionSubscript> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_SUBSCRIPT;

  using pattern =
  Seq<
    Ref<parse_expression_call>,
    Some<Seq<
      Operator<"[">,
      Ref<parse_expression>,
      Operator<"]">
    >>
  >;
};

const Token* parse_expression_subscript(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionSubscript,
    Ref<parse_expression_call>
  >;
  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionMember : public NodeMaker<NodeExpressionMember> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_MEMBER;

  using pattern =
  Seq<
    Ref<parse_expression_subscript>,
    Some<Seq<
      Oneof<
        NodeOperator<".">,
        NodeOperator<"->">
      >,
      Ref<parse_expression_subscript>
    >>
  >;
};

const Token* parse_expression_member(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionMember,
    Ref<parse_expression_subscript>
  >;
  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionPreIncDec : public NodeMaker<NodeExpressionPreIncDec> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_PREINCDEC;

  using pattern =
  Seq<
    Oneof<
      NodeOperator<"++">,
      NodeOperator<"--">
    >,
    Ref<parse_expression_preincdec>
  >;
};

const Token* parse_expression_preincdec(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionPreIncDec,
    Ref<parse_expression_member>
  >;
  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionPlusMinus : public NodeMaker<NodeExpressionPlusMinus> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_PLUSMINUS;

  using pattern =
  Seq<
    Oneof<
      NodeOperator<"+">,
      NodeOperator<"-">
    >,
    Ref<parse_expression_plusminus>
  >;
};

const Token* parse_expression_plusminus(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionPlusMinus,
    Ref<parse_expression_preincdec>
  >;
  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionNot : public NodeMaker<NodeExpressionNot> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_NOT;

  using pattern =
  Seq<
    Oneof<
      NodeOperator<"!">,
      NodeOperator<"~">
    >,
    Ref<parse_expression_not>
  >;
};

const Token* parse_expression_not(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionNot,
    Ref<parse_expression_plusminus>
  >;
  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionCast : public NodeMaker<NodeExpressionCast> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_CAST;

  using pattern =
  Seq<
    Atom<'('>,
    NodeDecltype,
    Atom<')'>,
    Ref<parse_expression_cast>
  >;
};

const Token* parse_expression_cast(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionCast,
    Ref<parse_expression_not>
  >;
  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionDereference : public NodeMaker<NodeExpressionDereference> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_DEREFERENCE;

  using pattern = Seq<
    NodeOperator<"*">,
    Ref<parse_expression_dereference>
  >;
};

const Token* parse_expression_dereference(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionDereference,
    Ref<parse_expression_cast>
  >;

  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionAddressof : public NodeMaker<NodeExpressionAddressof> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_ADDRESSOF;

  using pattern = Seq<
    NodeOperator<"&">,
    Ref<parse_expression_addressof>
  >;
};

const Token* parse_expression_addressof(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionAddressof,
    Ref<parse_expression_dereference>
  >;

  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionSizeof : public NodeMaker<NodeExpressionSizeof> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_SIZEOF;

  using pattern = Seq<
    Keyword<"sizeof">,
    Oneof<
      Seq<Atom<'('>, NodeDecltype, Atom<')'>>,
      Ref<parse_expression_sizeof>
    >
  >;
};

const Token* parse_expression_sizeof(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionSizeof,
    Ref<parse_expression_addressof>
  >;

  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionPtrToMember : public NodeMaker<NodeExpressionPtrToMember> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_PTRTOMEMBER;

  using pattern = Seq<
    Ref<parse_expression_sizeof>,
    Some<Seq<
      Oneof<
        NodeOperator<"->*">,
        NodeOperator<".*">
      >,
      Ref<parse_expression_sizeof>
    >>
  >;
};

const Token* parse_expression_ptrtomember(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionPtrToMember,
    Ref<parse_expression_sizeof>
  >;

  return pattern::match(a, b);
}

//------------------------------------------------------------------------------
// FIXME associativity is wrong

struct NodeExpressionMulDivMod : public NodeMaker<NodeExpressionMulDivMod> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_MULDIVMOD;

  using pattern = Seq<
    Ref<parse_expression_ptrtomember>,
    Some<Seq<
      Oneof<
        NodeOperator<"*">,
        NodeOperator<"/">,
        NodeOperator<"%">
      >,
      Ref<parse_expression_ptrtomember>
    >>
  >;
};

const Token* parse_expression_muldivmod(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionMulDivMod,
    Ref<parse_expression_ptrtomember>
  >;

  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionAddSub : public NodeMaker<NodeExpressionAddSub> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_ADDSUB;

  using pattern = Seq<
    Ref<parse_expression_muldivmod>,
    Some<Seq<
      Oneof<
        NodeOperator<"+">,
        NodeOperator<"-">
      >,
      Ref<parse_expression_muldivmod>
    >>
  >;
};

const Token* parse_expression_addsub(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionAddSub,
    Ref<parse_expression_muldivmod>
  >;

  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionShift : public NodeMaker<NodeExpressionShift> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_SHIFT;

  using pattern = Seq<
    Ref<parse_expression_addsub>,
    Some<Seq<
      Oneof<
        NodeOperator<"<<">,
        NodeOperator<">>">
      >,
      Ref<parse_expression_addsub>
    >>
  >;
};

const Token* parse_expression_shift(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionShift,
    Ref<parse_expression_addsub>
  >;

  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionThreeway : public NodeMaker<NodeExpressionThreeway> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_THREEWAY;

  using pattern = Seq<
    Ref<parse_expression_shift>,
    Some<Seq<
      NodeOperator<"<=>">,
      Ref<parse_expression_shift>
    >>
  >;
};

const Token* parse_expression_threeway(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionThreeway,
    Ref<parse_expression_shift>
  >;

  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionRelational : public NodeMaker<NodeExpressionRelational> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_RELATIONAL;

  using pattern = Seq<
    Ref<parse_expression_threeway>,
    Some<Seq<
      Oneof<
        NodeOperator<"<=">,
        NodeOperator<">=">,
        NodeOperator<"<">,
        NodeOperator<">">
      >,
      Ref<parse_expression_threeway>
    >>
  >;
};

const Token* parse_expression_relational(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionRelational,
    Ref<parse_expression_threeway>
  >;

  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionEquality : public NodeMaker<NodeExpressionEquality> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_EQUALITY;

  using pattern = Seq<
    Ref<parse_expression_relational>,
    Some<Seq<
      Oneof<
        NodeOperator<"==">,
        NodeOperator<"!=">
      >,
      Ref<parse_expression_relational>
    >>
  >;
};

const Token* parse_expression_equality(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionEquality,
    Ref<parse_expression_relational>
  >;

  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionBitwiseAnd : public NodeMaker<NodeExpressionBitwiseAnd> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_BITWISE_AND;

  using pattern = Seq<
    Ref<parse_expression_equality>,
    Some<Seq<
      NodeOperator<"&">,
      Ref<parse_expression_bitwise_and>
    >>
  >;
};

const Token* parse_expression_bitwise_and(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionBitwiseAnd,
    Ref<parse_expression_equality>
  >;

  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionBitwiseXor : public NodeMaker<NodeExpressionBitwiseXor> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_BITWISE_XOR;

  using pattern = Seq<
    Ref<parse_expression_bitwise_and>,
    Some<Seq<
      NodeOperator<"^">,
      Ref<parse_expression_bitwise_and>
    >>
  >;
};

const Token* parse_expression_bitwise_xor(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionBitwiseXor,
    Ref<parse_expression_bitwise_and>
  >;

  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionBitwiseOr : public NodeMaker<NodeExpressionBitwiseOr> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_BITWISE_OR;

  using pattern = Seq<
    Ref<parse_expression_bitwise_xor>,
    Some<Seq<
      NodeOperator<"|">,
      Ref<parse_expression_bitwise_xor>
    >>
  >;
};

const Token* parse_expression_bitwise_or(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionBitwiseOr,
    Ref<parse_expression_bitwise_xor>
  >;

  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionLogicalAnd : public NodeMaker<NodeExpressionLogicalAnd> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_LOGICAL_AND;

  using pattern = Seq<
    Ref<parse_expression_bitwise_or>,
    Some<Seq<
      NodeOperator<"&&">,
      Ref<parse_expression_bitwise_or>
    >>
  >;
};

const Token* parse_expression_logical_and(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionLogicalAnd,
    Ref<parse_expression_bitwise_or>
  >;

  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionLogicalOr : public NodeMaker<NodeExpressionLogicalOr> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_LOGICAL_OR;

  using pattern = Seq<
    Ref<parse_expression_logical_and>,
    Some<Seq<
      NodeOperator<"||">,
      Ref<parse_expression_logical_and>
    >>
  >;
};

const Token* parse_expression_logical_or(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionLogicalOr,
    Ref<parse_expression_logical_and>
  >;

  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionTernary : public NodeMaker<NodeExpressionTernary> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_TERNARY;

  using pattern = Seq<
    Ref<parse_expression_logical_or>,
    NodeOperator<"?">,
    Ref<parse_expression_ternary>,
    NodeOperator<":">,
    Ref<parse_expression_ternary>
  >;
};

const Token* parse_expression_ternary(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionTernary,
    Ref<parse_expression_logical_or>
  >;

  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionAssignment : public NodeMaker<NodeExpressionAssignment> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_ASSIGNMENT;

  using pattern =
  Seq<
    Ref<parse_expression_ternary>,
    Some<Seq<
      Oneof<
        NodeOperator<"<<=">,
        NodeOperator<">>=">,
        NodeOperator<"+=">,
        NodeOperator<"-=">,
        NodeOperator<"*=">,
        NodeOperator<"/=">,
        NodeOperator<"%=">,
        NodeOperator<"&=">,
        NodeOperator<"|=">,
        NodeOperator<"^=">,
        NodeOperator<"=">
      >,
      Ref<parse_expression_ternary>
    >>
  >;
};

const Token* parse_expression_assignment(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionAssignment,
    Ref<parse_expression_ternary>
  >;

  return pattern::match(a, b);
}

//------------------------------------------------------------------------------
// left-assoc

struct NodeExpressionComma : public NodeMaker<NodeExpressionComma> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_COMMA;

  using pattern = Seq<
    Ref<parse_expression_assignment>,
    Some<Seq<
      NodeOperator<",">,
      Ref<parse_expression_assignment>
    >>
  >;

};

const Token* parse_expression_comma(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionComma,
    Ref<parse_expression_assignment>
  >;

  return pattern::match(a, b);
}

//------------------------------------------------------------------------------
// We need a shunting yard algorithm....

const Token* parse_expression(const Token* a, const Token* b) {
  using pattern = Ref<parse_expression_comma>;

  return pattern::match(a, b);
}

//==============================================================================











































































//------------------------------------------------------------------------------

/*
struct NodeExpressionList : public NodeMaker<NodeExpressionList> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_LIST;

  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<NodeExpressionList>::match(a, b);
    return end;
  }

  using pattern = Oneof<
    Seq<
      Atom<'('>,
      opt_comma_separated<Ref<parse_expression>>,
      Atom<')'>
    >,
    Delimited<'{', '}', comma_separated<Ref<parse_expression>>>
  >;
};
*/

//------------------------------------------------------------------------------

struct NodeIfStatement : public NodeMaker<NodeIfStatement> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_STATEMENT_IFELSE;

  using pattern = Seq<
    Keyword<"if">,
    Seq<
      Atom<'('>,
      Ref<parse_expression>,
      Atom<')'>
    >,
    Ref<parse_statement>,
    Opt<Seq<
      Keyword<"else">,
      Ref<parse_statement>
    >>
  >;
};

//------------------------------------------------------------------------------

struct NodeWhileStatement : public NodeMaker<NodeWhileStatement> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_STATEMENT_WHILE;

  using pattern = Seq<
    Keyword<"while">,
    Seq<
      Atom<'('>,
      Ref<parse_expression>,
      Atom<')'>
    >,
    Ref<parse_statement>
  >;
};

//------------------------------------------------------------------------------

struct NodeReturnStatement : public NodeMaker<NodeReturnStatement> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_STATEMENT_RETURN;

  using pattern = Seq<
    Keyword<"return">,
    Ref<parse_expression>,
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

struct NodePreproc : public NodeMaker<NodePreproc> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_PREPROC;

  using pattern = Atom<TOK_PREPROC>;
};

//------------------------------------------------------------------------------

struct NodeCaseStatement : public NodeMaker<NodeCaseStatement> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_STATEMENT_CASE;

  using pattern = Seq<
    Keyword<"case">,
    Ref<parse_expression>,
    Atom<':'>,
    Any<Seq<
      Not<Keyword<"case">>,
      Not<Keyword<"default">>,
      Ref<parse_statement>
    >>
  >;
};

//------------------------------------------------------------------------------

struct NodeDefaultStatement : public NodeMaker<NodeDefaultStatement> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_STATEMENT_DEFAULT;

  using pattern = Seq<
    Keyword<"default">,
    Atom<':'>,
    Any<Seq<
      Not<Keyword<"case">>,
      Not<Keyword<"default">>,
      Ref<parse_statement>
    >>
  >;
};

//------------------------------------------------------------------------------

struct NodeSwitchStatement : public NodeMaker<NodeSwitchStatement> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_STATEMENT_SWITCH;

  using pattern = Seq<
    Keyword<"switch">,
    Ref<parse_expression>,
    Atom<'{'>,
    Any<Oneof<
      NodeCaseStatement,
      NodeDefaultStatement
    >>,
    Atom<'}'>
  >;
};

//------------------------------------------------------------------------------

struct NodeForStatement : public NodeMaker<NodeForStatement> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_STATEMENT_FOR;

  using pattern = Seq<
    Keyword<"for">,
    Atom<'('>,
    Opt<Oneof<
      Ref<parse_expression>,
      NodeDeclaration
    >>,
    Atom<';'>,
    Opt<Ref<parse_expression>>,
    Atom<';'>,
    Opt<Ref<parse_expression>>,
    Atom<')'>,
    Ref<parse_statement>
  >;
};

//----------------------------------------

struct NodeExpressionStatement : public NodeMaker<NodeExpressionStatement> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_STATEMENT_EXPRESSION;

  using pattern = Seq<
    Ref<parse_expression>,
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

const Token* parse_statement(const Token* a, const Token* b) {
  using pattern_statement = Oneof<
    NodeCompoundStatement,
    NodeIfStatement,
    NodeWhileStatement,
    NodeForStatement,
    NodeReturnStatement,
    NodeSwitchStatement,
    NodeDeclarationStatement,
    NodeExpressionStatement
  >;

  return pattern_statement::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeTemplateDeclaration : public NodeMaker<NodeTemplateDeclaration> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_DECLARATION_TEMPLATE;

  using pattern = Seq<
    Keyword<"template">,
    NodeTemplateParams,
    NodeClass
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

struct NodeTranslationUnit : public NodeMaker<NodeTranslationUnit> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_TRANSLATION_UNIT;

  using pattern = Any<
    ProgressBar<
      Oneof<
        NodePreproc,
        NodeNamespace,
        NodeEnum,
        NodeTemplateDeclaration,
        NodeClass,
        NodeStruct,
        NodeFunction,
        NodeDeclarationStatement
      >
    >
  >;
};

//------------------------------------------------------------------------------







































































//==============================================================================

void lex_file(const std::string& path, std::string& text, std::vector<Lexeme>& lexemes, std::vector<Token>& tokens) {

  auto size = std::filesystem::file_size(path);

  text.resize(size + 1);
  memset(text.data(), 0, size + 1);
  FILE* file = fopen(path.c_str(), "rb");
  if (!file) {
    printf("Could not open %s!\n", path.c_str());
  }
  auto r = fread(text.data(), 1, size, file);

  auto text_a = text.data();
  auto text_b = text_a + size;

  const char* cursor = text_a;
  while(cursor) {
    auto lex = next_lexeme(cursor, text_b);
    lexemes.push_back(lex);
    if (lex.type == LEX_EOF) {
      break;
    }
    cursor = lex.span_b;
  }

  for (auto i = 0; i < lexemes.size(); i++) {
    auto l = &lexemes[i];
    if (!l->is_gap()) {
      tokens.push_back(Token(lex_to_tok(l->type), l));
    }
  }
}

//------------------------------------------------------------------------------

void dump_lexemes(const std::string& path, int size, std::string& text, std::vector<Lexeme>& lexemes) {
  for(auto& l : lexemes) {
    printf("%-15s ", l.str());

    int len = l.span_b - l.span_a;
    if (len > 80) len = 80;

    for (int i = 0; i < len; i++) {
      auto text_a = text.data();
      auto text_b = text_a + size;
      if (l.span_a + i >= text_b) {
        putc('#', stdout);
        continue;
      }

      auto c = l.span_a[i];
      if (c == '\n' || c == '\t' || c == '\r') {
        putc('@', stdout);
      }
      else {
        putc(l.span_a[i], stdout);
      }
    }
    //printf("%-.40s", l.span_a);
    printf("\n");

    if (l.is_eof()) break;
  }
}

//------------------------------------------------------------------------------

struct ExpressionTest : public NodeMaker<ExpressionTest> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_TEST;

  using pattern =
  Some<
    Seq<
      Ref<parse_expression>,
      Atom<';'>
    >
  >;
};

//------------------------------------------------------------------------------

int oneof_count = 0;

int test_c99_peg(int argc, char** argv) {
  printf("Parseroni Demo\n");


  std::vector<std::string> paths;
  const char* base_path = argc > 1 ? argv[1] : "tests";

  printf("Parsing source files in %s\n", base_path);
  using rdit = std::filesystem::recursive_directory_iterator;
  for (const auto& f : rdit(base_path)) {
    if (!f.is_regular_file()) continue;
    paths.push_back(f.path().native());
  }

  paths = { "tests/scratch.h" };
  //paths = { "mini_tests/csmith.cpp" };

  double lex_accum = 0;
  double parse_accum = 0;

  for (const auto& path : paths) {
    std::string text;
    std::vector<Lexeme> lexemes;
    std::vector<Token> tokens;

    printf("Lexing %s\n", path.c_str());

    lex_accum -= timestamp_ms();
    lex_file(path, text, lexemes, tokens);
    lex_accum += timestamp_ms();

    const Token* token_a = tokens.data();
    const Token* token_b = tokens.data() + tokens.size() - 1;

    printf("Parsing %s\n", path.c_str());

    parse_accum -= timestamp_ms();
    //NodeTranslationUnit::match(token_a, token_b);
    ExpressionTest::match(token_a, token_b);
    parse_accum += timestamp_ms();

    if (node_stack.top() != 1) {
      printf("Node stack wrong size %ld\n", node_stack._top);
      return -1;
    }

    auto root = node_stack.pop();

    root->dump_tree();

    if (root->tok_a != token_a) {
      printf("Root's first token is not token_a!\n");
    }
    else if (root->tok_b != token_b) {
      printf("Root's last token is not token_b!\n");
    }
    else {
      printf("Root OK\n");
    }

    delete root;

  }

  printf("Lexing took  %f msec\n", lex_accum);
  printf("Parsing took %f msec\n", parse_accum);

  printf("oneof_count = %d\n", oneof_count);

  return 0;
}

//------------------------------------------------------------------------------
