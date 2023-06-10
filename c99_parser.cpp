#include "Matcheroni.h"

#include "Lexemes.h"
#include "Node.h"
#include "NodeTypes.h"
#include "Tokens.h"

#include <filesystem>
#include <memory.h>

double timestamp_ms();

using namespace matcheroni;

bool atom_eq(const Token& a, const TokenType& b) {
  return a.type == b;
}

bool atom_eq(const Token& a, const LexemeType& b) {
  return a.lex->type == b;
}

bool atom_eq(const Token& a, const char& b) {
  return a.lex->len() == 1 && (*a.lex->span_a == b);
}

bool atom_lte(const Token& a, const char& b) {
  return a.lex->len() == 1 && (*a.lex->span_a <= b);
}

bool atom_gte(const Token& a, const char& b) {
  return a.lex->len() == 1 && (*a.lex->span_a >= b);
}

template<int N>
bool atom_eq(const Token& a, const matcheroni::StringParam<N>& b) {
  if (a.lex->len() != b.len) return false;
  for (auto i = 0; i < b.len; i++) {
    if (a.lex->span_a[i] != b.value[i]) return false;
  }

  return true;
}

//----------------------------------------

template<typename CharMatcher>
const Token* match_chars(const Token* a, const Token* b) {
  auto span_a = a->lex->span_a;
  auto span_b = b->lex->span_a;

  auto end = CharMatcher::match(span_a, span_b);
  if (!end) return nullptr;

  auto match_lex_a = a;
  auto match_lex_b = a;
  while(match_lex_b->lex->span_a < end) match_lex_b++;
  return match_lex_b;
}

//------------------------------------------------------------------------------

const char* c_specifiers[] = {
  "const",
  "constexpr",
  "extern",
  "inline",
  "register",
  "restrict",
  "signed",
  "static",
  "thread_local",
  "unsigned",
  "volatile",
  "_Noreturn",
};

const char* c_primitives[] = {
  "auto",
  "bool",
  "char",
  "double",
  "float",
  "int",
  "long",
  "short",
  "void",
};

const char* c_control[] = {
  "break",
  "case",
  "continue",
  "default",
  "do",
  "else",
  "for",
  "goto",
  "if",
  "return",
  "switch",
  "while",
};

const char* c_constants[] = {
  "false",
  "nullptr",
  "true",
};

const char* c_intrinsics[] = {
  "alignas",
  "alignof",
  "sizeof",
  "static_assert",
  "typeof",
  "typeof_unqual",
};

const char* c_structs[] = {
  "enum",
  "struct",
  "typedef",
  "union",
};

//------------------------------------------------------------------------------

Node* node_stack[256] = {0};
size_t node_top = 0;

Node* pop_node() {
  assert(node_top);
  return node_stack[--node_top];
}

void push_node(Node* n) {
  node_stack[node_top++] = n;
}

void dump_top() {
  node_stack[node_top-1]->dump_tree();
}

//----------------------------------------

template<typename pattern>
struct Dump {
  static const Token* match(const Token* a, const Token* b) {
    auto end = pattern::match(a, b);
    if (end) dump_top();
    return end;
  }
};

//----------------------------------------

const Token* parse_expression(const Token* a, const Token* b);
const Token* parse_statement(const Token* a, const Token* b);

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
struct comma_separated : public
Seq<
  P,
  Any<Seq<Atom<','>, P>>
> {};

template<typename P>
struct opt_comma_separated : public
Opt<Seq<
  P,
  Any<Seq<Atom<','>, P>>
>> {};

//------------------------------------------------------------------------------

template<NodeType n, typename pattern>
struct NodeMaker {
  static const Token* match(const Token* a, const Token* b) {
    auto old_top = node_top;
    auto end = pattern::match(a, b);
    if (end && end != a) {
      auto node = new Node(n, a, end);
      for (auto i = old_top; i < node_top; i++) {
        node->append(node_stack[i]);
      }
      node_top = old_top;
      push_node(node);
      return end;
    }
    else {
      for (auto i = old_top; i < node_top; i++) {
        delete node_stack[i];
      }
      node_top = old_top;
      return nullptr;
    }
  }
};

//----------------------------------------

struct pattern_any_identifier : public NodeMaker<
  NODE_IDENTIFIER,
  Atom<TOK_IDENTIFIER>
> {};

struct pattern_constant : public NodeMaker<
  NODE_CONSTANT,
  Oneof<
    Atom<TOK_FLOAT>,
    Atom<TOK_INT>,
    Atom<TOK_CHAR>,
    Atom<TOK_STRING>
  >
> {};

//----------------------------------------

template<char punct>
struct pattern_punct : public NodeMaker<NODE_PUNCT, Atom<punct>> {};

//----------------------------------------

class pattern_access_specifier : public NodeMaker<NODE_ACCESS_SPECIFIER,
  Seq<
    Oneof<AtomLit<"public">, AtomLit<"private">>,
    Atom<':'>
  >
> {};

//----------------------------------------

class pattern_initializer_list : public NodeMaker<NODE_INITIALIZER_LIST,
  Seq<
    Atom<':'>,
    comma_separated<Ref<parse_expression>>,
    And<Atom<'{'>>
  >
> {};

//----------------------------------------

struct match_specifier {
  static const Token* match(const Token* a, const Token* b) {
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

//----------------------------------------

struct match_qualifier {
  static const Token* match(const Token* a, const Token* b) {
    const char* keywords[] = {
      "const",
      "volatile",
      "restrict",
      "__restrict__",
      "_Atomic",
      "_Noreturn",
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

//----------------------------------------

struct pattern_specifier_list : public NodeMaker<
  NODE_SPECIFIER_LIST,
  Some<Oneof<
    NodeMaker<NODE_IDENTIFIER, match_specifier>,
    NodeMaker<NODE_PUNCT, Atom<'*'>>
  >>
> {};

//----------------------------------------

struct pattern_compound_statement : public NodeMaker<
  NODE_COMPOUND_STATEMENT,
  Seq<
    Atom<'{'>,
    Any<Ref<parse_statement>>,
    Atom<'}'>
  >
> {};

//----------------------------------------

struct pattern_enum_body : public NodeMaker<
  NODE_ENUMERATOR_LIST,
  Seq<
    Atom<'{'>,
    comma_separated<Ref<parse_expression>>,
    Atom<'}'>
  >
> {};

//----------------------------------------

struct pattern_template_argument_list : public NodeMaker<
  NODE_ARGUMENT_LIST,
  Delimited<'<', '>', Seq<
    opt_comma_separated<Ref<parse_expression>>
  >>
> {};

//----------------------------------------

struct pattern_decltype : public NodeMaker<
  NODE_DECLTYPE,
  Seq<
    pattern_any_identifier,
    Opt<pattern_template_argument_list>,
    Opt<Seq<
      Atom<':'>,
      Atom<':'>,
      pattern_any_identifier
    >>,
    Opt<Atom<'*'>>
  >
> {};

//----------------------------------------
// FIXME should probably have a few diffeerent versions instead of all the opts

struct pattern_enum_declaration : public NodeMaker<
  NODE_ENUM_DECLARATION,
  Seq<
    AtomLit<"enum">,
    Opt<AtomLit<"class">>,
    Opt<pattern_any_identifier>,
    Opt<Seq<Atom<':'>, pattern_decltype>>,
    Opt<pattern_enum_body>,
    Opt<pattern_any_identifier>
  >
> {};

//----------------------------------------

struct pattern_array_expression : public NodeMaker<
  NODE_ARRAY_EXPRESSION,
  Seq<
    Atom<'['>,
    Ref<parse_expression>,
    Atom<']'>
  >
> {};

//----------------------------------------

struct pattern_declaration : public NodeMaker<
  NODE_DECLARATION,
  Seq<
    Opt<pattern_specifier_list>,
    pattern_decltype,
    pattern_any_identifier,
    Opt<pattern_array_expression>,
    Opt<Seq<
      Atom<'='>,
      Ref<parse_expression>
    >>
  >
> {};


//----------------------------------------

struct pattern_parenthesized_declaration_list : public NodeMaker<
  NODE_PARAMETER_LIST,
  Seq<
    Atom<'('>,
    opt_comma_separated<pattern_declaration>,
    Atom<')'>
  >
> {};

//----------------------------------------

struct pattern_constructor : public NodeMaker<
  NODE_CONSTRUCTOR,
  Seq<
    pattern_any_identifier,
    pattern_parenthesized_declaration_list,
    Opt<pattern_initializer_list>,
    Opt<pattern_specifier_list>,
    Oneof<
      pattern_compound_statement,
      Atom<';'>
    >
  >
> {};

//----------------------------------------

struct pattern_function_definition : public NodeMaker<
  NODE_FUNCTION_DEFINITION,
  Seq<
    pattern_decltype,
    pattern_any_identifier,
    pattern_parenthesized_declaration_list,
    Opt<AtomLit<"const">>,
    Oneof<
      pattern_compound_statement,
      Atom<';'>
    >
  >
> {};

//----------------------------------------

struct pattern_assignment : public NodeMaker<
  NODE_ASSIGNMENT_EXPRESSION,
  Seq<
    pattern_any_identifier,
    Ref<match_chars<Ref<match_assign_op>>>,
    Ref<parse_expression>
  >
> {};

//----------------------------------------

struct pattern_field_declaration : public Oneof<
  pattern_constructor,
  pattern_enum_declaration,
  pattern_function_definition,
  pattern_declaration,
  pattern_assignment
> {};

//----------------------------------------

struct pattern_field_declaration_list : public NodeMaker<NODE_FIELD_DECLARATION_LIST,
  Seq<
    Atom<'{'>,
    Any<Oneof<
      pattern_access_specifier,
      Seq<pattern_field_declaration,Opt<Atom<';'>>>
    >>,
    Atom<'}'>
  >
> {};

//----------------------------------------

struct pattern_class_specifier : public NodeMaker<
  NODE_CLASS_DECLARATION,
  Seq<
    AtomLit<"class">,
    pattern_any_identifier,
    Opt<pattern_field_declaration_list>,
    Atom<';'>>
> {};

//----------------------------------------

struct pattern_struct_specifier : NodeMaker<
  NODE_STRUCT_DECLARATION,
  Seq<
    AtomLit<"struct">,
    pattern_any_identifier,
    Opt<pattern_field_declaration_list>,
    Atom<';'>>
> {};

//----------------------------------------

struct pattern_namespace_specifier : NodeMaker<
  NODE_NAMESPACE_DECLARATION,
  Seq<
    AtomLit<"namespace">,
    pattern_any_identifier,
    Opt<pattern_field_declaration_list>,
    Atom<';'>>
> {};

//----------------------------------------

struct pattern_prefix_op : public NodeMaker<
  NODE_OPERATOR,
  Ref<match_chars<Ref<match_prefix_op>>>
> {};

struct pattern_typecast : public NodeMaker<
  NODE_TYPECAST,
  Seq<Atom<'('>, Atom<LEX_IDENTIFIER>, Atom<')'> >
> {};

struct pattern_sizeof : public NodeMaker<
  NODE_OPERATOR,
  AtomLit<"sizeof">
> {};

struct pattern_expression_prefix : public Oneof<
  pattern_prefix_op,
  pattern_typecast,
  pattern_sizeof
> {};

//----------------------------------------

struct pattern_parenthesized_expression_list : public NodeMaker<
  NODE_ARGUMENT_LIST,
  Delimited<'(', ')',
    opt_comma_separated<Ref<parse_expression>>
  >
> {};

struct pattern_array_suffix : public NodeMaker<NODE_ARRAY_SUFFIX,
  Seq<Atom<'['>, Opt<Ref<parse_expression>>, Atom<']'>>
> {};

struct pattern_inc : public NodeMaker<
  NODE_OPERATOR,
  Seq<Atom<'+'>, Atom<'+'>>
> {};

struct pattern_dec : public NodeMaker<
  NODE_OPERATOR,
  Seq<Atom<'-'>, Atom<'-'>>
> {};

struct pattern_expression_suffix : public Oneof<
  pattern_template_argument_list,
  pattern_parenthesized_expression_list,
  pattern_array_suffix,
  pattern_inc,
  pattern_dec
> {};

//----------------------------------------

struct pattern_infix_op : public NodeMaker<NODE_OPERATOR,
  Ref<match_chars<Ref<match_infix_op>>>
> {};

//----------------------------------------

struct pattern_parenthesized_expression : public NodeMaker<
  NODE_PARENTHESIZED_EXPRESSION,
  Seq<
    Atom<'('>,
    Ref<parse_expression>,
    Atom<')'>
  >
> {};

//----------------------------------------

struct pattern_function_call : public NodeMaker<
  NODE_CALL_EXPRESSION,
  Seq<
    pattern_any_identifier,
    Opt<pattern_template_argument_list>,
    pattern_parenthesized_expression_list
  >
> {};

//----------------------------------------

struct pattern_template_parameter_list : public NodeMaker<
  NODE_TEMPLATE_PARAMETER_LIST,
  Delimited<'<', '>', Seq<
    pattern_declaration,
    Any<Seq<Atom<','>, pattern_declaration>>
  >>
> {};

//----------------------------------------

struct pattern_expression : public Seq<
  Any<pattern_expression_prefix>,
  Oneof<
    pattern_parenthesized_expression,
    pattern_constant,
    pattern_function_call,
    pattern_any_identifier
  >,
  Any<pattern_expression_suffix>,
  Opt<Seq<
    pattern_infix_op,
    Ref<parse_expression>
  >>
> {};

const Token* parse_expression(const Token* a, const Token* b) {
  return pattern_expression::match(a, b);
}

//----------------------------------------

struct pattern_external_declaration : public Oneof<
  pattern_namespace_specifier,
  pattern_class_specifier,
  pattern_struct_specifier,
  Seq<pattern_declaration, Atom<';'>>
> {};

//----------------------------------------

struct pattern_if_statement : public NodeMaker<
  NODE_IF_STATEMENT,
  Seq<
    AtomLit<"if">,
    pattern_parenthesized_expression,
    Ref<parse_statement>,
    Opt<Seq<
      AtomLit<"else">,
      Ref<parse_statement>
    >>
  >
> {};

//----------------------------------------

struct pattern_while_statement : public NodeMaker<
  NODE_WHILE_STATEMENT,
  Seq<
    AtomLit<"while">,
    pattern_parenthesized_expression,
    Ref<parse_statement>
  >
> {};

//----------------------------------------

struct pattern_return_statement : public NodeMaker<
  NODE_RETURN_STATEMENT,
  Seq<
    AtomLit<"return">,
    pattern_expression,
    Atom<';'>
  >
> {};

//----------------------------------------

struct pattern_preproc : public NodeMaker<
  NODE_PREPROC,
  Atom<TOK_PREPROC>
> {};

//----------------------------------------

struct pattern_case_body : public
Any<Seq<
  Not<AtomLit<"case">>,
  Not<AtomLit<"default">>,
  Ref<parse_statement>
>> {};

struct pattern_case_statement : public
NodeMaker<
  NODE_CASE_STATEMENT,
  Seq<
    AtomLit<"case">,
    pattern_expression,
    Atom<':'>,
    pattern_case_body
  >
> {};

struct pattern_default_statement : public
NodeMaker<
  NODE_DEFAULT_STATEMENT,
  Seq<
    AtomLit<"default">,
    Atom<':'>,
    pattern_case_body
  >
> {};

struct pattern_case_or_default : public
Oneof<
  pattern_case_statement,
  pattern_default_statement
> {};

struct pattern_switch_statement : public
NodeMaker<
  NODE_SWITCH_STATEMENT,
  Seq<
    AtomLit<"switch">,
    pattern_parenthesized_expression,
    Atom<'{'>,
    Any<pattern_case_or_default>,
    Atom<'}'>
  >
> {};

//----------------------------------------

struct pattern_declaration_or_expression : public Oneof<
  pattern_declaration,
  pattern_expression
> {};

//----------------------------------------

struct pattern_for_statement : public NodeMaker<
  NODE_FOR_STATEMENT,
  Seq<
    AtomLit<"for">,
    Atom<'('>,
    Opt<pattern_declaration_or_expression>,
    Atom<';'>,
    Opt<pattern_expression>,
    Atom<';'>,
    Opt<pattern_expression>,
    Atom<')'>,
    Ref<parse_statement>
  >
> {};

//----------------------------------------

struct pattern_declaration_statement : public NodeMaker<
  NODE_DECLARATION_STATEMENT,
  Seq<
    pattern_declaration,
    Atom<';'>
  >
> {};

//----------------------------------------

struct pattern_expression_statement : public NodeMaker<
  NODE_EXPRESSION_STATEMENT,
  Seq<
    pattern_expression,
    Atom<';'>
  >
> {};

//----------------------------------------
// _Does_ include the semicolon for single-line statements

struct pattern_statement : public Oneof<
  pattern_compound_statement,
  pattern_if_statement,
  pattern_while_statement,
  pattern_for_statement,
  pattern_return_statement,
  pattern_switch_statement,
  pattern_declaration_statement,
  pattern_expression_statement
> {};

const Token* parse_statement(const Token* a, const Token* b) {
  return pattern_statement::match(a, b);
}

//----------------------------------------

struct pattern_template_decl : public NodeMaker<
  NODE_TEMPLATE_DECLARATION,
  Seq<
    AtomLit<"template">,
    pattern_template_parameter_list,
    pattern_class_specifier
  >
> {};

//----------------------------------------

struct pattern_translation_unit : public NodeMaker<
  NODE_TRANSLATION_UNIT,
  Any<Oneof<
    pattern_template_decl,
    pattern_preproc,
    pattern_external_declaration
  >>
> {};








































































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

  //paths = { "tests/bit_casts.h" };

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
    pattern_translation_unit::match(token, token_eof);
    parse_accum += timestamp_ms();

    assert(node_top == 1);
    //dump_top();

    auto root = pop_node();
    delete root;

  }

  printf("Lexing took  %f msec\n", lex_accum);
  printf("Parsing took %f msec\n", parse_accum);

  return 0;
}

//------------------------------------------------------------------------------
