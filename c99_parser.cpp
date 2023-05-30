#include "Matcheroni.h"
#include <assert.h>
#include <stdio.h>
#include <memory.h>
#include "c_lexer.h"
#include "Node.h"
#include "NodeTypes.h"

#include <stack>
#include <vector>

using namespace matcheroni;

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

Node* parse_type_specifier_qualifier(Lexeme* a, Lexeme* b) {
  return nullptr;
}

//------------------------------------------------------------------------------



Node* parse_declaration_specifier(const Lexeme* a, const Lexeme* b) {
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

  auto end = declaration_specifier::match(a, b, nullptr);
  if (end) {
    auto result = new Node(NODE_DECLARATION_SPECIFIER, a, end);
    return result;
  }

  return nullptr;
}

//------------------------------------------------------------------------------

Node* parse_declaration_specifiers(const Lexeme* a, const Lexeme* b) {
  //using declaration_specifiers = Some<Oneof<declaration_specifier, type_specifier_qualifier> >;
  //return declaration_specifiers::match(a, b, ctx);

  auto result = new Node(NODE_DECLARATION_SPECIFIERS, nullptr, nullptr);

  while(a < b) {
    if (auto child = parse_declaration_specifier(a, b)) {
      result->append(child);
      a = child->lex_b;
    }
  }

  return nullptr;
}

//------------------------------------------------------------------------------
/*
using function_definition = Seq<
  Opt<attribute_specifier_sequence>,
  declaration_specifiers,
  declarator,
  function_body
>;
*/

Node* parse_function_definition(const Lexeme* a, const Lexeme* b) {
  auto declaration_specifiers = parse_declaration_specifiers(a, b);
  if (!declaration_specifiers) return nullptr;

  return nullptr;
}

//------------------------------------------------------------------------------
/*
(6.7) declaration:
  declaration-specifiers init-declarator-listopt ;
  attribute-specifier-sequence declaration-specifiers init-declarator-list ;
  static_assert-declaration
  attribute-declaration
*/

Node* parse_declaration(const Lexeme* a, const Lexeme* b) {
  return nullptr;
}

//------------------------------------------------------------------------------
/*
(6.9) external-declaration:
  function-definition
  declaration
*/

Node* parse_external_declaration(const Lexeme* a, const Lexeme* b) {
  if (auto func = parse_function_definition(a, b)) {
    return func;
  }
  else if (auto decl = parse_declaration(a, b)) {
    return decl;
  }
  else {
    return nullptr;
  }
}

//------------------------------------------------------------------------------
/*
(6.9) translation-unit:
  external-declaration
  translation-unit external-declaration
*/

Node* parse_translation_unit(const Lexeme* a, const Lexeme* b) {
  auto result = new TranslationUnit(a, b);

  for(auto cursor = a; cursor < b; cursor++) {
    if (auto decl = parse_external_declaration(a, b)) {
      result->decls.push_back(decl);
    }
    else {
      return nullptr;
    }
  }

  return nullptr;
}

//------------------------------------------------------------------------------

void bluh() {
  const char* t = "foobarbaz";
  Lexeme a[3] = {
    { LEX_PUNCT, &t[0], &t[3] },
    { LEX_PUNCT, &t[3], &t[6] },
    { LEX_PUNCT, &t[6], &t[9] },
  };

  using pattern = Oneof<AtomLit<"bar">, AtomLit<"foo">>;

  const Lexeme* result = pattern::match(&a[0], &a[3], nullptr);

  printf("%p\n", result);
}

//------------------------------------------------------------------------------

int test_c99_peg() {
  printf("Hello World\n");

  bluh();

  auto text_a = text;
  auto text_b = text + strlen(text);

  auto cursor = text_a;

  std::vector<Lexeme> lexemes;

  while(cursor) {
    auto t = next_lexeme(cursor, text_b);
    lexemes.push_back(t);
    if (t.lexeme == LEX_EOF) break;
    cursor = t.span_b;
  }

  if (0) {
    for(auto& l : lexemes) {
      printf("%-15s", l.str());
      printf("\n");
    }
  }

  auto a = lexemes.data();
  auto b = a + lexemes.size();

  /*
  while(cursor < text_b) {
    auto t = next_token(cursor, text_b);
    if (t) {
      auto n = new Node(t.lex, TOK_INVALID, t.span_a, t.span_b);
      root->append_lex(n);
      if (!n->is_gap()) root->append_tok(n);
      cursor = t.span_b;
    }
    else {
      break;
    }
  }
  */

  printf("//----------------------------------------\n");

  //dump_tree(root, 100);

  printf("//----------------------------------------\n");

  return 0;
}

//------------------------------------------------------------------------------
