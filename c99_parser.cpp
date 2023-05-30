#include "Matcheroni.h"
#include <assert.h>
#include <stdio.h>
#include <memory.h>
#include "c_lexer.h"
#include "Node.h"
#include "NodeTypes.h"

#include <stack>
#include <vector>
#include <filesystem>

double timestamp_ms();

using namespace matcheroni;

bool atom_eq(const Lexeme& a, const LexemeType& b) {
  return a.lexeme == b;
}

bool atom_eq(const Lexeme& a, const char& b) {
  return a.len() == 1 && *a.span_a == b;
}

bool atom_lte(const Lexeme& a, const char& b) {
  return a.len() == 1 && *a.span_a <= b;
}

bool atom_gte(const Lexeme& a, const char& b) {
  return a.len() == 1 && *a.span_a >= b;
}

template<int N>
bool atom_eq(const Lexeme& a, const matcheroni::StringParam<N>& b) {
  if ((a.span_b - a.span_a) != b.len) return false;
  for (auto i = 0; i < b.len; i++) {
    if (a.span_a[i] != b.value[i]) return false;
  }

  return true;
}

//------------------------------------------------------------------------------

const char* text = R"(
#include <stdio.h>

// This is a comment.

int main(int argc, char** argv) {
  // Another comment
  printf("Hello () World %d %p!\n", 12345, argv);
  return 0;
}

)";

//------------------------------------------------------------------------------

Node* fold_nodes(Node* a, Node* b, NodeType tok) {
  auto parent = a->parent;

  auto n = new Node(tok, a->lex_a, b->lex_b);

  n->parent = nullptr;
  n->prev = a->prev;
  n->next = b->next;
  n->head = a;
  n->tail = b;

  if (n->prev) n->prev->next = n;
  if (n->next) n->next->prev = n;

  if (parent->head == a) parent->head = n;
  if (parent->tail == b) parent->tail = n;

  a->prev = nullptr;
  b->next = nullptr;

  for (auto c = a; c; c = c->next) c->parent = n;

  return n;
}

//------------------------------------------------------------------------------

void fold_delims(Node* a, Node* b, char ldelim, char rdelim, NodeType block_type) {

  std::stack<Node*> ldelims;

  for (Node* cursor = a; cursor; cursor = cursor->next) {
    if (cursor->lex_a->lexeme != LEX_PUNCT) continue;

    if (*cursor->lex_a->span_a == ldelim) {
      ldelims.push(cursor);
    }
    else if (*cursor->lex_a->span_a == rdelim) {
      auto node_ldelim = ldelims.top();
      auto node_rdelim = cursor;
      cursor = fold_nodes(node_ldelim, node_rdelim, block_type);
      ldelims.pop();
    }
  }
}

//------------------------------------------------------------------------------

struct Parser {
  const Lexeme* lex_a;
  const Lexeme* lex_b;

  //----------------------------------------

  NodeType lex_to_node(LexemeType t) {
    switch(t) {
      case LEX_STRING: return NODE_CONSTANT;
      case LEX_IDENTIFIER: return NODE_IDENTIFIER;
      case LEX_PREPROC: return NODE_PREPROC;
      case LEX_FLOAT: return NODE_CONSTANT;
      case LEX_INT: return NODE_CONSTANT;
      case LEX_PUNCT: return NODE_PUNCT;
      case LEX_CHAR: return NODE_CONSTANT;
    }
    assert(false);
    return NODE_INVALID;
  }

  //----------------------------------------

  Node* take_lexeme() {
    auto result = new Node(lex_to_node(lex_a->lexeme), lex_a, lex_a + 1);
    lex_a = lex_a + 1;
    return result;
  }

  Node* take_punct() {
    assert (lex_a->is_punct());
    return take_lexeme();
  }

  void skip_punct(char punct) {
    assert (lex_a->is_punct(punct));
    lex_a++;
  }

  Node* take_punct(char punct) {
    assert (lex_a->is_punct(punct));
    return take_lexeme();
  }

  Node* take_identifier(const char* identifier = nullptr) {
    assert(lex_a->is_identifier(identifier));
    return take_lexeme();
  }

  Node* take_constant() {
    assert(lex_a->is_constant());
    return take_lexeme();
  }

  //----------------------------------------
  /*
  (6.9) translation-unit:
    external-declaration
    translation-unit external-declaration
  */

  Node* parse_translation_unit() {
    auto result = new TranslationUnit();

    while(lex_a != lex_b) {
      if (Lit<"template">::match(lex_a->span_a, lex_a->span_b, nullptr)) {
        result->append(parse_template_decl());
      }
      if (auto decl = parse_preproc()) {
        result->append(decl);
      }
      else if (auto decl = parse_external_declaration()) {
        result->append(decl);
      }
      else {
        assert(false);
        break;
      }
    }

    return result;
  }

  //----------------------------------------

  Node* parse_typeof_specifier() {
    Node* result = nullptr;
    // FIXME
    //using typeof_specifier_argument = Oneof<expression, type_name>;

    using typeof_specifier_argument = Atom<LEX_IDENTIFIER>;

    using typeof_specifier = Oneof<
      Seq< AtomLit<"typeof">, Atom<'('>, typeof_specifier_argument, Atom<')'> >,
      Seq< AtomLit<"typeof_unqual">, Atom<'('>, typeof_specifier_argument, Atom<')'> >
    >;

    if (auto end = typeof_specifier::match(lex_a, lex_b, nullptr)) {
      result = new Node(NODE_TYPEOF_SPECIFIER, lex_a, end);
      lex_a = end;
    }

    return result;
  }

  //----------------------------------------

  Node* parse_typedef_name() {
    Node* result = nullptr;
    using typedef_name = Atom<LEX_IDENTIFIER>;

    if (auto end = typedef_name::match(lex_a, lex_b, nullptr)) {
      result = new Node(NODE_TYPEDEF_NAME, lex_a, end);
      lex_a = end;
    }

    return result;
  }

  //----------------------------------------

  Node* parse_type_specifier(Lexeme* a, Lexeme* b) {
    /*
    type-specifier:
      void
      char
      short
      int
      long
      float
      double
      signed
      unsigned
      _BitInt ( constant-expression )
      bool
      _Complex
      _Decimal32
      _Decimal64
      _Decimal128
      atomic-type-specifier
      struct-or-union-specifier
      enum-specifier
      typedef-name
      typeof-specifier
    */

    return nullptr;
  }

  //----------------------------------------

  Node* parse_type_qualifier() {
    assert(false);
    return nullptr;
  }

  //----------------------------------------

  Node* parse_type_specifier_qualifier() {
    assert(false);
    return nullptr;
  }

  //----------------------------------------

  Node* parse_declaration_specifier() {
    Node* result = nullptr;

    // 6.7.1 Storage-class specifiers
    using storage_class_specifier = Oneof<
      AtomLit<"auto">,
      AtomLit<"constexpr">,
      AtomLit<"extern">,
      AtomLit<"register">,
      AtomLit<"static">,
      AtomLit<"thread_local">,
      AtomLit<"typedef">
    >;

    // 6.7.4 Function specifiers
    using function_specifier = Oneof<
      AtomLit<"inline">,
      AtomLit<"_Noreturn">
    >;

    using declaration_specifier = Oneof<
      storage_class_specifier,
      //type_specifier_qualifier,
      function_specifier
    >;

    if (auto end = declaration_specifier::match(lex_a, lex_b, nullptr)) {
      result = new Node(NODE_DECLARATION_SPECIFIER, lex_a, end);
      lex_a = end;
    }

    return result;
  }

  //----------------------------------------

  Node* parse_declaration_specifiers() {
    //using declaration_specifiers = Some<Oneof<declaration_specifier, type_specifier_qualifier> >;
    //return declaration_specifiers::match(a, b, ctx);

    auto result = new Node(NODE_DECLARATION_SPECIFIERS, nullptr, nullptr);

    while(1) {
      if (auto child = parse_declaration_specifier()) {
        result->append(child);
      }
      else {
        break;
      }
    }

    if (result->head) return result;

    return nullptr;
  }

  //----------------------------------------

  Node* parse_declarator() {
    assert(false);
    return nullptr;
  }

  //----------------------------------------
  /*
  using function_definition = Seq<
    Opt<attribute_specifier_sequence>,
    declaration_specifiers,
    declarator,
    function_body
  >;
  */

  Node* parse_function_definition() {
    auto old_a = lex_a;

    auto declaration_specifiers = parse_declaration_specifiers();
    if (!declaration_specifiers) {
      lex_a = old_a;
      return nullptr;
    }

    auto declarator = parse_declarator();
    if (!declarator) {
      lex_a = old_a;
      return nullptr;
    }

    return nullptr;
  }

  //----------------------------------------

  //----------------------------------------
  /*
  (6.9) external-declaration:
    function-definition
    declaration
  */

  Node* parse_external_declaration() {
    if (auto func = parse_function_definition()) {
      return func;
    }
    else if (auto decl = parse_declaration()) {
      return decl;
    }
    else {
      return nullptr;
    }
  }

  //----------------------------------------

  Node* parse_identifier() {
    if (lex_a->lexeme != LEX_IDENTIFIER) return nullptr;
    auto result = new Node(NODE_DECLTYPE, lex_a, lex_a + 1);
    lex_a = lex_a + 1;
    return result;
  }

  Node* parse_decltype() {
    // FIXME placeholder
    if (lex_a->lexeme != LEX_IDENTIFIER) return nullptr;
    auto result = new Node(NODE_DECLTYPE, lex_a, lex_a + 1);
    lex_a = lex_a + 1;
    return result;
  }

  //----------------------------------------

  Node* parse_expression() {
    if (lex_a->is_constant()) {
      return take_constant();
    }
    else {
      assert(false);
      return nullptr;
    }
  }

  //----------------------------------------
  /*
  (6.7) declaration:
    declaration-specifiers init-declarator-listopt ;
    attribute-specifier-sequence declaration-specifiers init-declarator-list ;
    static_assert-declaration
    attribute-declaration
  */
  Node* parse_declaration() {
    // not following the spec here right now
    auto result = new Declaration();

    result->append(parse_decltype());
    result->append(parse_identifier());
    result->_decltype = result->child(0);
    result->_declname = result->child(1);

    if (lex_a->is_punct('=')) {
      result->append(take_punct());
      result->append(parse_expression());
      result->_declop   = result->child(2);
      result->_declinit = result->child(3);
    }

    return result;
  }

  //----------------------------------------

  Node* parse_template_decl() {
    auto result = new TemplateDeclaration();

    result->append(take_identifier("template"));
    result->append(parse_template_parameter_list());
    result->append(parse_class_specifier());

    result->_keyword = result->child(0);
    result->_params  = result->child(1);
    result->_class   = result->child(2);

    return result;
  }

  //----------------------------------------

  Node* parse_template_parameter_list() {

    Node* result = new Node(NODE_TEMPLATE_PARAMETER_LIST, nullptr, nullptr);

    skip_punct('<');

    while(1) {
      result->append(parse_declaration());
      if (lex_a->is_punct('>')) {
        skip_punct('>');
        break;
      }
      else if (lex_a->is_punct(',')) {
        skip_punct(',');
      }
      else {
        assert(false);
      }
    }

    return result;
  }

  //----------------------------------------

  Node* parse_parameter_list() {
    Node* result = new ParameterList();

    skip_punct('(');

    while(!lex_a->is_punct(')')) {
      result->append(parse_declaration());
      if (!lex_a->is_punct(',')) break;
      skip_punct(',');
    }

    return result;

    assert(false);
    return nullptr;
  }

  //----------------------------------------

  Node* parse_statement() {
    assert(false);
    return nullptr;
  }

  //----------------------------------------

  Node* parse_compound_statement() {
    CompoundStatement* result = new CompoundStatement();

    skip_punct('{');

    while(1) {
      if (lex_a->is_punct('}')) break;
      result->append(parse_statement());
    }

    assert(false);
    return nullptr;
  }

  //----------------------------------------
  /*
  LEX_IDENTIFIER  Module

  LEX_PUNCT       (
  LEX_IDENTIFIER  const
  LEX_IDENTIFIER  char
  LEX_PUNCT       *
  LEX_IDENTIFIER  filename
  LEX_PUNCT       =
  LEX_STRING      "examples/uart/message.hex"
  LEX_PUNCT       ,
  LEX_IDENTIFIER  logic
  LEX_PUNCT       <
  LEX_INT         10
  LEX_PUNCT       >
  LEX_IDENTIFIER  start_addr
  LEX_PUNCT       =
  LEX_INT         0
  LEX_PUNCT       )

  LEX_PUNCT       {
  LEX_IDENTIFIER  addr
  LEX_PUNCT       =
  LEX_IDENTIFIER  start_addr
  LEX_PUNCT       ;
  LEX_IDENTIFIER  readmemh
  LEX_PUNCT       (
  LEX_IDENTIFIER  filename
  LEX_PUNCT       ,
  LEX_IDENTIFIER  data
  LEX_PUNCT       )
  LEX_PUNCT       ;
  LEX_PUNCT       }
  */

  //----------------------------------------

  Node* parse_constructor() {
    auto result = new Constructor();

    // Constructor
    result->_decl   = take_identifier();
    result->_params = parse_parameter_list();

    if (lex_a->is_punct('{')) {
      result->_body = parse_compound_statement();
    }
    else if (lex_a->is_punct(';')) {
      skip_punct(';');
    }
    else {
      assert(false);
    }

    assert(false);
    return nullptr;
  }

  //----------------------------------------

  Node* parse_field_declaration() {
    if (lex_a[0].is_identifier() && lex_a[1].is_punct('(')) {
      return parse_constructor();
    }
    else {
      assert(false);
    }
  }

  //----------------------------------------

  Node* parse_access_specifier() {
    using pattern = Seq<
      Oneof<AtomLit<"public">, AtomLit<"private">>,
      Atom<':'>
    >;

    if (auto end = pattern::match(lex_a, lex_b, nullptr)) {
      Node* result = new Node(NODE_ACCESS_SPECIFIER, lex_a, end);
      lex_a = end;
      return result;
    }
    assert(false);
    return nullptr;
  }

  //----------------------------------------

  Node* parse_field_declaration_list() {
    Node* result = new Node(NODE_FIELD_DECLARATION_LIST, nullptr, nullptr);

    skip_punct('{');
    while(lex_a < lex_b) {
      if (auto child = parse_access_specifier()) {
        delete child;
        //result->append(child);
      }
      else if (auto child = parse_field_declaration()) {
        result->append(child);
      }
    }
    return result;
  }

  //----------------------------------------

  Node* parse_class_specifier() {
    Node* result = new Node(NODE_CLASS_SPECIFIER, nullptr, nullptr);
    result->append(take_identifier("class"));
    result->append(take_identifier());
    result->append(parse_field_declaration_list());
    return result;
  }


  //----------------------------------------

  Node* parse_preproc() {
    using pattern = Atom<LEX_PREPROC>;

    if (auto end = pattern::match(lex_a, lex_b, nullptr)) {
      auto result = new Node(NODE_PREPROC, lex_a, end);
      lex_a = end;
      return result;
    }

    return nullptr;
  }
};

//------------------------------------------------------------------------------

void lex_file(const std::string& path, int size, std::string& text, std::vector<Lexeme>& lexemes) {
  printf("Lexing %s\n", path.c_str());
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
    auto t = next_lexeme(cursor, text_b);
    if (!t.is_gap()) {
      lexemes.push_back(t);
    }
    if (t.lexeme == LEX_EOF) break;
    cursor = t.span_b;
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
  }
}

//------------------------------------------------------------------------------

int test_c99_peg() {
  printf("Hello World\n");

  using rdit = std::filesystem::recursive_directory_iterator;

  //const char* base_path = "tests";
  const char* base_path = "mini_tests";

  printf("Parsing source files in %s\n", base_path);
  auto time_a = timestamp_ms();

  for (const auto& f : rdit(base_path)) {
    if (!f.is_regular_file()) continue;

    auto& path = f.path().native();
    auto size = f.file_size();
    std::string text;
    std::vector<Lexeme> lexemes;

    lex_file(path, size, text, lexemes);

    dump_lexemes(path, size, text, lexemes);

    Parser p;
    p.lex_a = lexemes.data();
    p.lex_b = p.lex_a + lexemes.size();

    auto root = p.parse_translation_unit();
    root->dump_tree();
    delete root;

  }

  auto time_b = timestamp_ms();

  printf("Parsing took %f msec\n", time_b - time_a);

  return 0;
}

//------------------------------------------------------------------------------
