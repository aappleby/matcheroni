
#if 0
TOKEN RING
top ptr
alt ptr
bulldozing forward clears top
like packrat
never delete a match because it's still a match'
#endif

#pragma once
#include <typeinfo>
#include <set>
#include <string>

#include "Lexemes.h"

struct ParseNode;
void set_color(uint32_t c);

//------------------------------------------------------------------------------
// Tokens associate lexemes with parse nodes.
// Tokens store bookkeeping data during parsing.

struct Token {
  Token(const Lexeme* lex) {
    this->top = nullptr;
    this->alt = nullptr;
    this->lex = lex;
  }

  bool is_valid() const {
    return lex->type != LEX_INVALID;
  }

  bool is_punct() const {
    return lex->is_punct();
  }

  bool is_punct(char p) const {
    return lex->is_punct(p);
  }

  bool is_lit(const char* lit) const {
    return lex->is_lit(lit);
  }

  bool is_identifier(const char* lit = nullptr) const {
    return lex->is_identifier(lit);
  }

  ParseNode* top;
  ParseNode* alt;
  const Lexeme* lex;
};

//------------------------------------------------------------------------------

inline int atom_cmp(Token& a, const LexemeType& b) {
  a.top = nullptr;
  return int(a.lex->type) - int(b);
}

inline int atom_cmp(Token& a, const char& b) {
  a.top = nullptr;
  int len_cmp = a.lex->len() - 1;
  if (len_cmp != 0) return len_cmp;
  return int(a.lex->span_a[0]) - int(b);
}

template<int N>
inline bool atom_cmp(Token& a, const StringParam<N>& b) {
  a.top = nullptr;
  int len_cmp = int(a.lex->len()) - int(b.len);
  if (len_cmp != 0) return len_cmp;

  for (auto i = 0; i < b.len; i++) {
    int cmp = int(a.lex->span_a[i]) - int(b.value[i]);
    if (cmp) return cmp;
  }

  return 0;
}

//------------------------------------------------------------------------------

struct ParseNode {

  inline static int constructor_count = 0;
  inline static int destructor_count = 0;
  inline static int instance_count = 0;

  ParseNode() {
    instance_count++;
    constructor_count++;
  }

  virtual ~ParseNode() {
    //printf("Deleting parsenode\n");

    instance_count--;
    destructor_count++;

    auto cursor = head;
    while(cursor) {
      auto next = cursor->next;
      cursor->parent = nullptr;
      cursor->prev = nullptr;
      cursor->next = nullptr;
      cursor = next;
    }
  }

  ParseNode* top() {
    if (parent) return parent->top();
    return this;
  }

  //----------------------------------------

  template<typename P>
  bool isa() const {
    if (this == nullptr) return false;
    return typeid(*this) == typeid(P);
  }

  template<typename P>
  P* child() {
    if (this == nullptr) return nullptr;
    for (auto cursor = head; cursor; cursor = cursor->next) {
      if (cursor->isa<P>()) {
        return dynamic_cast<P*>(cursor);
      }
    }
    return nullptr;
  }

  template<typename P>
  const P* child() const {
    if (this == nullptr) return nullptr;
    for (auto cursor = head; cursor; cursor = cursor->next) {
      if (cursor->isa<P>()) {
        return dynamic_cast<const P*>(cursor);
      }
    }
    return nullptr;
  }

  template<typename P>
  P* search() {
    if (this == nullptr) return nullptr;
    if (this->isa<P>()) return this->as<P>();
    for (auto cursor = head; cursor; cursor = cursor->next) {
      if (auto c = cursor->search<P>()) {
        return c;
      }
    }
    return nullptr;
  }

  template<typename P>
  P* as() {
    return dynamic_cast<P*>(this);
  };

  template<typename P>
  const P* as() const {
    return dynamic_cast<const P*>(this);
  };

  //----------------------------------------

  void append(ParseNode* node) {
    assert(node && node->parent == nullptr);
    node->parent = this;

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

  //----------------------------------------

  void sanity() {
    // All our children should be sane.
    for (auto cursor = head; cursor; cursor = cursor->next) {
      cursor->sanity();
    }

    // Our prev/next pointers should be hooked up correctly
    assert(!next || next->prev == this);
    assert(!prev || prev->next == this);

    ParseNode* cursor = nullptr;

    // Check node chain
    for (cursor = head; cursor && cursor->next; cursor = cursor->next);
    assert(cursor == tail);

    for (cursor = tail; cursor && cursor->prev; cursor = cursor->prev);
    assert(cursor == head);
  }

  //----------------------------------------

  Token* tok_a = nullptr;
  Token* tok_b = nullptr;

  ParseNode* alt    = nullptr;
  ParseNode* parent = nullptr;
  ParseNode* prev   = nullptr;
  ParseNode* next   = nullptr;
  ParseNode* head   = nullptr;
  ParseNode* tail   = nullptr;

  //----------------------------------------

  typedef std::set<std::string> IdentifierSet;

  inline static IdentifierSet builtin_types = {
    "void",
    "bool",
    "char", "short", "int", "long",
    "float", "double",
    "signed", "unsigned",
    "uint8_t", "uint16_t", "uint32_t", "uint64_t",
    "int8_t", "int16_t", "int32_t", "int64_t",
    "_Bool",
    "_Complex", // yes this is both a prefix and a type :P
    "__real__",
    "__imag__",
    "__builtin_va_list",
    "wchar_t",

    // technically part of the c library, but it shows up in stdarg test files
    "va_list",

    // used in fprintf.c torture test
    "FILE",

    // used in fputs-lib.c torture test
    "size_t",

    // pr60003.c fails if this is included, pr56982.c fails if it isn't
    //"jmp_buf",

    // gcc stuff
    "__int128",
    "__SIZE_TYPE__",
    "__PTRDIFF_TYPE__",
    "__WCHAR_TYPE__",
    "__WINT_TYPE__",
    "__INTMAX_TYPE__",
    "__UINTMAX_TYPE__",
    "__SIG_ATOMIC_TYPE__",
    "__INT8_TYPE__",
    "__INT16_TYPE__",
    "__INT32_TYPE__",
    "__INT64_TYPE__",
    "__UINT8_TYPE__",
    "__UINT16_TYPE__",
    "__UINT32_TYPE__",
    "__UINT64_TYPE__",
    "__INT_LEAST8_TYPE__",
    "__INT_LEAST16_TYPE__",
    "__INT_LEAST32_TYPE__",
    "__INT_LEAST64_TYPE__",
    "__UINT_LEAST8_TYPE__",
    "__UINT_LEAST16_TYPE__",
    "__UINT_LEAST32_TYPE__",
    "__UINT_LEAST64_TYPE__",
    "__INT_FAST8_TYPE__",
    "__INT_FAST16_TYPE__",
    "__INT_FAST32_TYPE__",
    "__INT_FAST64_TYPE__",
    "__UINT_FAST8_TYPE__",
    "__UINT_FAST16_TYPE__",
    "__UINT_FAST32_TYPE__",
    "__UINT_FAST64_TYPE__",
    "__INTPTR_TYPE__",
    "__UINTPTR_TYPE__",
  };

  inline static IdentifierSet builtin_type_prefixes = {
    "signed", "unsigned", "short", "long", "_Complex",
    "__signed__", "__unsigned__",
    "__complex__", "__real__", "__imag__",
  };

  inline static IdentifierSet builtin_type_suffixes = {
    // Why, GCC, why?
    "_Complex", "__complex__",
  };

  inline static IdentifierSet class_types;
  inline static IdentifierSet struct_types;
  inline static IdentifierSet union_types;
  inline static IdentifierSet enum_types;
  inline static IdentifierSet typedef_types;

  static void reset_types() {
    class_types.clear();
    struct_types.clear();
    union_types.clear();
    enum_types.clear();
    typedef_types.clear();
  }

  void print_class_name() {
    const char* name = typeid(*this).name();
    int name_len = 0;
    if (sscanf(name, "%d", &name_len)) {
      while((*name >= '0') && (*name <= '9')) name++;
      for (int i = 0; i < name_len; i++) {
        putc(name[i], stdout);
      }
    }
    else {
      printf("%s", name);
    }
  }
};

//------------------------------------------------------------------------------

template<typename NT>
struct NodeMaker : public ParseNode {

  static Token* match(Token* a, Token* b) {

    // See if there's a node on the token that we can reuse
    for (auto c = a->alt; c; c = c->alt) {
      if (typeid(*c) == typeid(NT)) {
        // Yep, there is - flip it to the top of the token and jump to its end
        printf("yay reuse\n");
        c->parent = nullptr;
        a->top = c;
        return c->tok_b;
      }
    }

    // No reusable node, make a new one if we match.

    auto end = NT::pattern::match(a, b);
    if (!end) {
      return nullptr;
    }

    auto node = new NT();
    node->tok_a = a;
    node->tok_b = end;

    // Append all the tops under the new node to it
    auto cursor = a;
    while (cursor < end) {
      if (cursor->top) {
        node->append(cursor->top);
        cursor = cursor->top->tok_b;
      }
      else {
        cursor++;
      }
    }

    // And now A's top becomes our new node
    a->top = node;
    node->alt = a->alt;
    a->alt = node;

    return end;
  }
};

//------------------------------------------------------------------------------

struct ParseNodeIterator {
  ParseNodeIterator(const ParseNode* cursor) : n(cursor) {}
  ParseNodeIterator& operator++() { n = n->next; return *this; }
  bool operator!=(const ParseNodeIterator& b) const { return n != b.n; }
  const ParseNode* operator*() const { return n; }
  const ParseNode* n;
};

inline ParseNodeIterator begin(const ParseNode* parent) {
  return ParseNodeIterator(parent->head);
}

inline ParseNodeIterator end(const ParseNode* parent) {
  return ParseNodeIterator(nullptr);
}

//------------------------------------------------------------------------------

inline Token* find_matching_delim(char ldelim, char rdelim, Token* a, Token* b) {
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
  static Token* match(Token* a, Token* b) {
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

struct NodeDispenser {

  NodeDispenser(ParseNode** children, size_t child_count) {
    this->children = children;
    this->child_count = child_count;
  }

  template<typename P>
  operator P*() {
    if (child_count == 0) return nullptr;
    if (children[0] == nullptr) return nullptr;
    if (children[0]->isa<P>()) {
      P* result = children[0]->as<P>();
      children++;
      child_count--;
      return result;
    }
    else {
      return nullptr;
    }
  }

  ParseNode** children;
  size_t child_count;
};

//------------------------------------------------------------------------------
