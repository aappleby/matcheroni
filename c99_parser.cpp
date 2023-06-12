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
      for (int i = old_top; i < new_top; i++) node_stack.pop();
      node_stack.push(node);
      return end;
    }
    else {
      for (auto i = old_top; i < new_top; i++) delete node_stack.pop();
      return nullptr;
    }
  }
};

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

const Token* parse_statement(const Token* a, const Token* b);

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

const Token* parse_expression(const Token* a, const Token* b);

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

struct NodeOperatorAssignment : public NodeMaker<NodeOperatorAssignment> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_OPERATOR;

  using pattern =
  Oneof<
    Operator<"<<=">,
    Operator<">>=">,
    Operator<"+=">,
    Operator<"-=">,
    Operator<"*=">,
    Operator<"/=">,
    Operator<"%=">,
    Operator<"&=">,
    Operator<"|=">,
    Operator<"^=">,
    Operator<"=">
  >;
};

struct NodeOperatorUnary : public NodeMaker<NodeOperatorUnary> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_OPERATOR;

  using pattern = Oneof<
    // pointlander's grammar doesn't have these in unary?
    //Operator<"++">,
    //Operator<"--">,
    Operator<"+">,
    Operator<"-">,
    Operator<"!">,
    Operator<"~">,
    Operator<"&">,
    Operator<"*">
  >;
};

struct NodeOperatorBinary : public NodeMaker<NodeOperatorBinary> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_OPERATOR;

  using pattern = Oneof <
    Operator<"<<=">,
    Operator<">>=">,
    Operator<"->">,
    Operator<"::">,
    Operator<"==">,
    Operator<"!=">,
    Operator<"<=">,
    Operator<">=">,
    Operator<"&&">,
    Operator<"||">,
    Operator<"<<">,
    Operator<">>">,
    Operator<"+=">,
    Operator<"-=">,
    Operator<"*=">,
    Operator<"/=">,
    Operator<"%=">,
    Operator<"&=">,
    Operator<"|=">,
    Operator<"^=">,
    Operator<"+">,
    Operator<"-">,
    Operator<"*">,
    Operator<"/">,
    Operator<"%">,
    Operator<"<">,
    Operator<">">,
    Operator<"&">,
    Operator<"|">,
    Operator<"^">,
    Operator<"=">,
    Operator<".">
  >;
};


#if 0
//------------------------------------------------------------------------------

struct NodeAssignment : public NodeMaker<NodeAssignment> {
  using NodeMaker::NodeMaker;
  static constexpr NodeType node_type = NODE_EXPRESSION_ASSIGN;

  using pattern = Seq<
    NodeIdentifier,
    NodeOperatorAssignment,
    Ref<parse_expression>
  >;
};

//------------------------------------------------------------------------------

struct NodeTypecast : public NodeMaker<NodeTypecast> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_TYPECAST;

  using pattern = Seq<Atom<'('>, NodeDecltype, Atom<')'> >;
};

//------------------------------------------------------------------------------

struct NodeSizeof : public NodeMaker<NodeSizeof> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_OPERATOR;

  using pattern = Keyword<"sizeof">;
};

//------------------------------------------------------------------------------

struct NodeInc : public NodeMaker<NodeInc> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_OPERATOR;

  using pattern = Operator<"++">;
};

//------------------------------------------------------------------------------

struct NodeDec : public NodeMaker<NodeDec> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_OPERATOR;

  using pattern = Operator<"--">;
};

//------------------------------------------------------------------------------

struct NodeCallExpression : public NodeMaker<NodeCallExpression> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_CALL;

  using pattern = Seq<
    NodeIdentifier,
    Opt<NodeTemplateArgs>,
    NodeExpressionList
  >;
};


//------------------------------------------------------------------------------
// FIXME this is a crappy way to build expression trees :/

struct NodeExpression : public NodeMaker<NodeExpression> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION;

  static const Token* match(const Token* a, const Token* b) {
    auto end = NodeMaker<NodeExpression>::match(a, b);
    return end;
  }

  // ok we're mistaking (foo) as a typecast

  // prefix_ops = { "++", "--", "+", "-", "!", "~", "&", "*" };
  // typecast   = Seq<Atom<'('>, NodeDecltype, Atom<')'> >;
  // sizeof     = Keyword<"sizeof">;

  using pattern = Seq<
    Any<Oneof<
      NodeOperatorUnary,
      NodeTypecast,
      NodeSizeof
    >>,
    Oneof<
      NodeExpressionList,
      NodeConstant,
      NodeCallExpression,
      NodeIdentifier
    >,
    Any<Oneof<
      NodeTemplateArgs,
      NodeExpressionList,
      NodeArraySuffix,
      NodeInc,
      NodeDec
    >>,
    Opt<Seq<
      NodeOperatorBinary,
      Ref<parse_expression>
    >>
  >;
};
#endif

//------------------------------------------------------------------------------

const Token* parse_expression_unary       (const Token* a, const Token* b);
const Token* parse_expression_binary      (const Token* a, const Token* b);
const Token* parse_expression_cast        (const Token* a, const Token* b);
const Token* parse_expression_assignment  (const Token* a, const Token* b);
const Token* parse_expression_postfix     (const Token* a, const Token* b);
const Token* parse_expression_conditional (const Token* a, const Token* b);
const Token* parse_expression_list        (const Token* a, const Token* b);

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

//------------------------------------------------------------------------------

/*
struct NodeExpressionPrimary : public NodeMaker<NodeExpressionPrimary> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_PRIMARY;

  using pattern = Oneof<
    NodeConstant,
    NodeIdentifier,
    NodeExpressionParenthesized
  >;
};
*/

using pattern_expression_primary = Oneof<
  NodeConstant,
  NodeIdentifier,
  NodeExpressionParenthesized
>;

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
  using pattern_expression_cast = Oneof<
    NodeExpressionCast,
    Ref<parse_expression_unary>
  >;
  return pattern_expression_cast::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionPostfix : public NodeMaker<NodeExpressionPostfix> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_POSTFIX;

  using pattern = Seq<
    pattern_expression_primary,
    Some<Oneof<
        Seq< Atom<'['>, Ref<parse_expression>, Atom<']'>>,
        Seq< Atom<'('>, Opt<Ref<parse_expression>>, Atom<')'>>,
        Seq< Operator<".">,  NodeIdentifier >,
        Seq< Operator<"->">, NodeIdentifier >,
        Operator<"++">,
        Operator<"--">
    >>
  >;
};

const Token* parse_expression_postfix(const Token* a, const Token* b) {
  using pattern =
  Oneof<
    NodeExpressionPostfix,
    pattern_expression_primary
  >;

  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionUnary : public NodeMaker<NodeExpressionUnary> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_UNARY;

  using pattern =
  Oneof<
    Seq<Operator<"++">, Ref<parse_expression_unary>>,
    Seq<Operator<"--">, Ref<parse_expression_unary>>,
    Seq<NodeOperatorUnary,  NodeExpressionCast>,
    Seq<
      Keyword<"sizeof">,
      Oneof<
        Ref<parse_expression_unary>,
        Seq<Atom<'('>, NodeDecltype, Atom<')'>>
      >
    >
  >;
};

const Token* parse_expression_unary(const Token* a, const Token* b) {
  using pattern =
  Oneof<
    NodeExpressionUnary,
    Ref<parse_expression_postfix>
  >;

  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionBinary : public NodeMaker<NodeExpressionBinary> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_BINARY;

  using pattern = Seq<
    Ref<parse_expression_cast>,
    NodeOperatorBinary,
    Ref<parse_expression_binary>
  >;
};

const Token* parse_expression_binary(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionBinary,
    Ref<parse_expression_cast>
  >;

  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionConditional : public NodeMaker<NodeExpressionConditional> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_CONDITIONAL;

  using pattern = Seq<
    Ref<parse_expression_binary>,
    Atom<'?'>,
    Ref<parse_expression_conditional>,
    Atom<':'>,
    Ref<parse_expression_conditional>
  >;
};

const Token* parse_expression_conditional(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionConditional,
    Ref<parse_expression_binary>
  >;

  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionAssignment : public NodeMaker<NodeExpressionAssignment> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_ASSIGN;

  using pattern =
  Seq<
    Ref<parse_expression_unary>,
    NodeOperatorAssignment,
    Ref<parse_expression_assignment>
  >;
};

const Token* parse_expression_assignment(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionAssignment,
    Ref<parse_expression_conditional>
  >;

  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeExpressionList : public NodeMaker<NodeExpressionList> {
  using NodeMaker::NodeMaker;
  static const NodeType node_type = NODE_EXPRESSION_LIST;

  using pattern = Seq<
    Ref<parse_expression_assignment>,
    Some<Seq<
      Atom<','>,
      Ref<parse_expression_assignment>
    >>
  >;

};

const Token* parse_expression_list(const Token* a, const Token* b) {
  using pattern = Oneof<
    NodeExpressionList,
    Ref<parse_expression_assignment>
  >;

  return pattern::match(a, b);
}

//------------------------------------------------------------------------------

const Token* parse_expression(const Token* a, const Token* b) {
  using pattern = Oneof<
    Ref<parse_expression_list>,
    Ref<parse_expression_assignment>
  >;

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
  static const NodeType node_type = NODE_EXPRESSION_LIST;

  using pattern =
  Some<
    Seq<
      Ref<parse_expression>,
      Atom<';'>
    >
  >;
};

//------------------------------------------------------------------------------

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

  return 0;
}

//------------------------------------------------------------------------------
