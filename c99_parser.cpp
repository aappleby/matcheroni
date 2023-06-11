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

template<typename P>
struct match_chars_as_tokens {
  static const Token* match(const Token* a, const Token* b) {
    auto span_a = a->lex->span_a;
    auto span_b = b->lex->span_a;

    auto end = P::match(span_a, span_b);
    if (!end) return nullptr;

    auto match_lex_a = a;
    auto match_lex_b = a;
    while(match_lex_b->lex->span_a < end) match_lex_b++;
    return match_lex_b;
  }
};

//------------------------------------------------------------------------------

struct NodeStack {
  Node* pop() {
    assert(_top);
    Node* result = _stack[_top - 1];
    _stack[_top - 1] = nullptr;
    _top--;
    return result;
  }

  Node* at(size_t i) {
    assert(i >= 0 && i < _top);
    return _stack[i];
  }

  size_t top() const { return _top; }

  void push(Node* n) {
    assert(_stack[_top] == nullptr);
    _stack[_top++] = n;
  }

  void dump_top() {
    _stack[_top-1]->dump_tree();
  }

  Node*  _stack[256] = {0};
  size_t _top = 0;
};

NodeStack node_stack;


//----------------------------------------

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

const Token* find_matching_delim(const Token* a, const Token* b) {
  char ldelim = *a->lex->span_a;
  a++;

  char rdelim = 0;
  if (ldelim == '<')  rdelim = '>';
  if (ldelim == '(')  rdelim = ')';
  if (ldelim == '{')  rdelim = '}';
  if (ldelim == '[')  rdelim = ']';
  if (!rdelim) return nullptr;

  while(a && a < b) {
    if (a->is_punct(rdelim)) return a;

    if (a->is_punct('<') ||
        a->is_punct('(') ||
        a->is_punct('{') ||
        a->is_punct('[')) {
      a = find_matching_delim(a, b);
      if (!a) return nullptr;
    }
    a++;
  }

  return nullptr;
}

//----------------------------------------

template<char ldelim, char rdelim, typename P>
struct Delimited {
  static const Token* match(const Token* a, const Token* b) {
    if (!a || !a->is_punct(ldelim)) return nullptr;
    auto new_b = find_matching_delim(a, b);
    if (!new_b || !new_b->is_punct(rdelim)) return nullptr;

    if (!new_b) return nullptr;
    if (auto end = P::match(a + 1, new_b)) {
      assert(end == new_b);
      return new_b + 1;
    }
    else {
      return nullptr;
    }
  }
};

template<typename P>
using comma_separated = Seq<P,Any<Seq<Atom<','>, P>>>;

template<typename P>
using opt_comma_separated = Opt<comma_separated<P>>;

//------------------------------------------------------------------------------

template<typename NT, typename P>
struct NodeMaker3 {
  static const Token* match(const Token* a, const Token* b) {
    auto old_top = node_stack.top();
    auto end = P::match(a, b);
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

template<typename NT>
struct NodeBase : public Node {
  NodeBase(const Token* a, const Token* b, Node** children, size_t child_count)
  : Node(NT::node_type, a, b, children, child_count) {
  }

  static const Token* match(const Token* a, const Token* b) {
    return NodeMaker3<NT, typename NT::pattern>::match(a, b);
  }
};

//------------------------------------------------------------------------------

struct NodeIdentifier : public NodeBase<NodeIdentifier> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_IDENTIFIER;

  using pattern = Atom<TOK_IDENTIFIER>;
};

//------------------------------------------------------------------------------

struct NodeConstant : public NodeBase<NodeConstant> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_CONSTANT;

  using pattern = Oneof<
    Atom<TOK_FLOAT>,
    Atom<TOK_INT>,
    Atom<TOK_CHAR>,
    Atom<TOK_STRING>
  >;
};

//------------------------------------------------------------------------------

struct match_specifier {
  static const Token* match(const Token* a, const Token* b) {
    if (auto end = Atom<'*'>::match(a, b)) {
      return end;
    }

    const char* keywords[] = {
      "extern",
      "static",
      "register",
      "inline",
      "thread_local",
      "const",
      "volatile",
      "restrict",
      "__restrict__",
      "_Atomic",
      "_Noreturn",
      "mutable",
      "constexpr",
      "constinit",
      "consteval",
      "virtual",
      "explicit",
    };
    auto keyword_count = sizeof(keywords)/sizeof(keywords[0]);

    for (auto i = 0; i < keyword_count; i++) {
      if (a->is_identifier(keywords[i])) {
        return a + 1;
      }
    }
    return nullptr;
  }
};

struct NodeSpecifier : public NodeBase<NodeSpecifier> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_SPECIFIER;

  using pattern = match_specifier;
};

//------------------------------------------------------------------------------

struct NodeSpecifierList : public NodeBase<NodeSpecifierList> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_SPECIFIER_LIST;

  using pattern = Some<NodeSpecifier>;
};

//------------------------------------------------------------------------------

const Token* parse_statement(const Token* a, const Token* b);

struct NodeCompoundStatement : public NodeBase<NodeCompoundStatement> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_COMPOUND_STATEMENT;

  using pattern = Seq<
    Atom<'{'>,
    Any<Ref<parse_statement>>,
    Atom<'}'>
  >;
};

//------------------------------------------------------------------------------

const Token* parse_expression(const Token* a, const Token* b);

struct NodeTemplateArgs : public NodeBase<NodeTemplateArgs> {
  using NodeBase::NodeBase;
  static constexpr NodeType node_type = NODE_ARGUMENT_LIST;

  using pattern = Delimited<'<', '>',
    opt_comma_separated<Ref<parse_expression>>
  >;
};

//------------------------------------------------------------------------------

struct NodeDecltype : public NodeBase<NodeDecltype> {
  using NodeBase::NodeBase;
  static constexpr NodeType node_type = NODE_DECLTYPE;

  using pattern = Seq<
    NodeIdentifier,
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

struct NodeEnum : public NodeBase<NodeEnum> {
  using NodeBase::NodeBase;
  static constexpr NodeType node_type = NODE_ENUM_DECLARATION;

  using pattern = Seq<
    Keyword<"enum">,
    Opt<Keyword<"class">>,
    Opt<NodeIdentifier>,
    Opt<Seq<Atom<':'>, NodeDecltype>>,
    //Opt<pattern_enum_body>,
    Opt<Seq<
      Atom<'{'>,
      comma_separated<Ref<parse_expression>>,
      Atom<'}'>
    >>,
    Opt<NodeIdentifier>
  >;
};

//------------------------------------------------------------------------------

struct NodeArrayExpression : public NodeBase<NodeArrayExpression> {
  using NodeBase::NodeBase;
  static constexpr NodeType node_type = NODE_ARRAY_EXPRESSION;

  using pattern = Seq<
    Atom<'['>,
    Ref<parse_expression>,
    Atom<']'>
  >;
};

//------------------------------------------------------------------------------

struct NodeInitializer : public NodeBase<NodeInitializer> {
  using NodeBase::NodeBase;
  static constexpr NodeType node_type = NODE_INITIALIZER;

  using pattern = Seq<Atom<'='>, Ref<parse_expression>>;
};

//------------------------------------------------------------------------------

struct NodeDeclaration : public NodeBase<NodeDeclaration> {
  using NodeBase::NodeBase;
  static constexpr NodeType node_type = NODE_DECLARATION;

  using pattern = Seq<
    Opt<NodeSpecifierList>,
    NodeDecltype,
    NodeIdentifier,
    Opt<NodeArrayExpression>,
    Opt<NodeInitializer>
  >;
};

//------------------------------------------------------------------------------

struct NodeDeclarationStatement : public NodeBase<NodeDeclarationStatement> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_DECLARATION_STATEMENT;

  using pattern = Seq<
    NodeDeclaration,
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

struct NodeDeclList : public NodeBase<NodeDeclList> {
  using NodeBase::NodeBase;
  static constexpr NodeType node_type = NODE_PARAMETER_LIST;

  using pattern = Seq<
    Atom<'('>,
    opt_comma_separated<NodeDeclaration>,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeInitializerList : public NodeBase<NodeInitializerList> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_INITIALIZER_LIST;

  using pattern = Seq<
    Atom<':'>,
    comma_separated<Ref<parse_expression>>,
    And<Atom<'{'>>
  >;
};

struct NodeConstructor : public NodeBase<NodeConstructor> {
  using NodeBase::NodeBase;
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

struct NodeFunction : public NodeBase<NodeFunction> {
  //using NodeBase::NodeBase;
  NodeFunction(const Token* a, const Token* b, Node** children, size_t child_count)
  : NodeBase(a, b, children, child_count) {
    _type = children[0]->as<NodeDecltype>();
    _name = children[1]->as<NodeIdentifier>();
    _args = children[2]->as<NodeDeclList>();
    _body = children[3]->as<NodeCompoundStatement>();
  }

  NodeDecltype*   _type = nullptr;
  NodeIdentifier* _name = nullptr;
  NodeDeclList*   _args = nullptr;
  NodeCompoundStatement* _body = nullptr;

  static constexpr NodeType node_type = NODE_FUNCTION_DEFINITION;

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

struct NodeAssignment : public NodeBase<NodeAssignment> {
  using NodeBase::NodeBase;
  static constexpr NodeType node_type = NODE_ASSIGNMENT_EXPRESSION;

  using pattern = Seq<
    NodeIdentifier,
    match_chars_as_tokens<Ref<match_assign_op>>,
    Ref<parse_expression>
  >;
};

//------------------------------------------------------------------------------

struct NodeAccessSpecifier : public NodeBase<NodeAccessSpecifier> {
  using NodeBase::NodeBase;
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

struct NodeFieldList : public NodeBase<NodeFieldList> {
  using NodeBase::NodeBase;
  static constexpr NodeType node_type = NODE_FIELD_LIST;

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

struct NodeClass : public NodeBase<NodeClass> {
  using NodeBase::NodeBase;
  static constexpr NodeType node_type = NODE_CLASS_DECLARATION;

  using pattern = Seq<
    Keyword<"class">,
    NodeIdentifier,
    Opt<NodeFieldList>,
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

struct NodeStruct : public NodeBase<NodeStruct> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_STRUCT_DECLARATION;

  using pattern = Seq<
    Keyword<"struct">,
    NodeIdentifier,
    Opt<NodeFieldList>,
    Atom<';'>
  >;
};

//----------------------------------------

struct NodeNamespace : public NodeBase<NodeNamespace> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_NAMESPACE_DECLARATION;

  using pattern = Seq<
    Keyword<"namespace">,
    NodeIdentifier,
    Opt<NodeFieldList>,
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

struct NodePrefixOp : public NodeBase<NodePrefixOp> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_OPERATOR;

  using pattern = match_chars_as_tokens<Ref<match_prefix_op>>;
};

//------------------------------------------------------------------------------

struct NodeTypecast : public NodeBase<NodeTypecast> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_TYPECAST;

  using pattern = Seq<Atom<'('>, Atom<TOK_IDENTIFIER>, Atom<')'> >;
};

//------------------------------------------------------------------------------

struct NodeSizeof : public NodeBase<NodeSizeof> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_OPERATOR;

  using pattern = Keyword<"sizeof">;
};

//------------------------------------------------------------------------------

struct NodeExpressionList : public NodeBase<NodeExpressionList> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_EXPRESSION_LIST;

  using pattern = Seq<
    Atom<'('>,
    opt_comma_separated<Ref<parse_expression>>,
    Atom<')'>
  >;
};

//------------------------------------------------------------------------------

struct NodeInc : public NodeBase<NodeInc> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_OPERATOR;

  using pattern = Seq<Atom<'+'>, Atom<'+'>>;
};

//------------------------------------------------------------------------------

struct NodeDec : public NodeBase<NodeDec> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_OPERATOR;

  using pattern = Seq<Atom<'-'>, Atom<'-'>>;
};

//------------------------------------------------------------------------------

struct NodeInfixOp : public NodeBase<NodeInfixOp> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_OPERATOR;

  using pattern = match_chars_as_tokens<Ref<match_infix_op>>;
};

//------------------------------------------------------------------------------

struct NodeCallExpression : public NodeBase<NodeCallExpression> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_CALL_EXPRESSION;

  using pattern = Seq<
    NodeIdentifier,
    Opt<NodeTemplateArgs>,
    NodeExpressionList
  >;
};

//------------------------------------------------------------------------------

struct NodeTemplateParams : public NodeBase<NodeTemplateParams> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_TEMPLATE_PARAMETER_LIST;

  using pattern = Delimited<'<', '>', comma_separated<NodeDeclaration>>;
};

//------------------------------------------------------------------------------
// FIXME this is a crappy way to build expression trees :/

struct NodeExpression : public NodeBase<NodeExpression> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_EXPRESSION;

  using pattern = Seq<
    Any<Oneof<
      NodePrefixOp,
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
      NodeArrayExpression,
      NodeInc,
      NodeDec
    >>,
    Opt<Seq<
      NodeInfixOp,
      Ref<parse_expression>
    >>
  >;
};

const Token* parse_expression(const Token* a, const Token* b) {
  return NodeExpression::match(a, b);
}

//------------------------------------------------------------------------------

struct NodeIfStatement : public NodeBase<NodeIfStatement> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_IF_STATEMENT;

  using pattern = Seq<
    Keyword<"if">,
    NodeExpressionList,
    Ref<parse_statement>,
    Opt<Seq<
      Keyword<"else">,
      Ref<parse_statement>
    >>
  >;
};

//------------------------------------------------------------------------------

struct NodeWhileStatement : public NodeBase<NodeWhileStatement> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_WHILE_STATEMENT;

  using pattern = Seq<
    Keyword<"while">,
    NodeExpressionList,
    Ref<parse_statement>
  >;
};

//------------------------------------------------------------------------------

struct NodeReturnStatement : public NodeBase<NodeReturnStatement> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_RETURN_STATEMENT;

  using pattern = Seq<
    Keyword<"return">,
    NodeExpression,
    Atom<';'>
  >;
};

//------------------------------------------------------------------------------

struct NodePreproc : public NodeBase<NodePreproc> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_PREPROC;

  using pattern = Atom<TOK_PREPROC>;
};

//------------------------------------------------------------------------------

struct NodeCaseStatement : public NodeBase<NodeCaseStatement> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_CASE_STATEMENT;

  using pattern = Seq<
    Keyword<"case">,
    NodeExpression,
    Atom<':'>,
    Any<Seq<
      Not<Keyword<"case">>,
      Not<Keyword<"default">>,
      Ref<parse_statement>
    >>
  >;
};

//------------------------------------------------------------------------------

struct NodeDefaultStatement : public NodeBase<NodeDefaultStatement> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_DEFAULT_STATEMENT;

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

struct NodeSwitchStatement : public NodeBase<NodeSwitchStatement> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_SWITCH_STATEMENT;

  using pattern = Seq<
    Keyword<"switch">,
    NodeExpressionList,
    Atom<'{'>,
    Any<Oneof<
      NodeCaseStatement,
      NodeDefaultStatement
    >>,
    Atom<'}'>
  >;
};

//------------------------------------------------------------------------------

struct NodeForStatement : public NodeBase<NodeForStatement> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_FOR_STATEMENT;

  using pattern = Seq<
    Keyword<"for">,
    Atom<'('>,
    Opt<Oneof<
      NodeDeclaration,
      NodeExpression
    >>,
    Atom<';'>,
    Opt<NodeExpression>,
    Atom<';'>,
    Opt<NodeExpression>,
    Atom<')'>,
    Ref<parse_statement>
  >;
};

//----------------------------------------

struct NodeExpressionStatement : public NodeBase<NodeExpressionStatement> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_EXPRESSION_STATEMENT;

  using pattern = Seq<
    NodeExpression,
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

struct NodeTemplateDeclaration : public NodeBase<NodeTemplateDeclaration> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_TEMPLATE_DECLARATION;

  using pattern = Seq<
    Keyword<"template">,
    NodeTemplateParams,
    NodeClass
  >;
};

//------------------------------------------------------------------------------

struct NodeTranslationUnit : public NodeBase<NodeTranslationUnit> {
  using NodeBase::NodeBase;
  static const NodeType node_type = NODE_TRANSLATION_UNIT;

  using pattern = Any<Oneof<
    NodePreproc,
    NodeNamespace,
    NodeTemplateDeclaration,
    NodeClass,
    NodeStruct,
    NodeDeclarationStatement
  >>;
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
    if (lex.type == LEX_EOF) {
      for (int i = 0; i < 10; i++) lexemes.push_back(lex);
      break;
    }
    else {
      lexemes.push_back(lex);
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

int test_c99_peg(int argc, char** argv) {
  printf("Parseroni Demo\n");

  using rdit = std::filesystem::recursive_directory_iterator;

  std::vector<std::string> paths;

  //const char* base_path = "tests";
  //const char* base_path = "mini_tests";
  const char* base_path = argc > 1 ? argv[1] : "tests";


  printf("Parsing source files in %s\n", base_path);
  for (const auto& f : rdit(base_path)) {
    if (!f.is_regular_file()) continue;
    paths.push_back(f.path().native());
  }

  //paths = { "tests/nested_submod_calls.h" };

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

    const Token* token = tokens.data();
    const Token* token_eof = tokens.data() + tokens.size() - 1;

    printf("Parsing %s\n", path.c_str());

    parse_accum -= timestamp_ms();
    NodeTranslationUnit::match(token, token_eof);
    parse_accum += timestamp_ms();

    assert(node_stack.top() == 1);
    //node_stack.dump_top();

    auto root = node_stack.pop();
    delete root;

  }

  printf("Lexing took  %f msec\n", lex_accum);
  printf("Parsing took %f msec\n", parse_accum);

  return 0;
}

//------------------------------------------------------------------------------
