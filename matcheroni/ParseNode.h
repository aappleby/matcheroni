
#pragma once
#include "Lexemes.h"
#include "SlabAlloc.h"
#include <typeinfo>
#include <assert.h>

struct Token;
struct ParseNode;

void dump_tree(const ParseNode* n, int max_depth, int indentation);
void set_color(uint32_t c);

#ifdef DEBUG
#define DCHECK(A) assert(A)
#else
#define DCHECK(A)
#endif

#define CHECK(A) assert(A)

//------------------------------------------------------------------------------
// Tokens associate lexemes with parse nodes.
// Tokens store bookkeeping data during parsing.

struct Token {
  Token(const Lexeme* lex) {
    this->lex  = lex;
    this->span = nullptr;
  }

  //----------------------------------------
  // These methods REMOVE THE SPAN FROM THE NODE - that's why they're not
  // const. This is required to ensure that if an Opt<> matcher fails to match
  // a branch, when it tries to match the next branch we will always pull the
  // defunct nodes off the tokens.

  LexemeType get_type() {
    span = nullptr;
    return lex->type;
  }

  const char* as_str() {
    span = nullptr;
    return lex->span_a;
  }

  const char* span_a() {
    span = nullptr;
    return lex->span_a;
  }

  const char* span_b() {
    span = nullptr;
    return lex->span_b;
  }

  int get_len() {
    span = nullptr;
    return int(lex->span_b - lex->span_a);
  }

  //----------------------------------------

  ParseNode* get_span() {
    return span;
  }

  void set_span(ParseNode* n) {
    span = n;
  }

  const char* type_to_str() const {
    return lex->type_to_str();
  }

  uint32_t type_to_color() const {
    return lex->type_to_color();
  }

  void dump_token() const;

  //----------------------------------------

  const char* debug_span_a() const { return lex->span_a; }
  const char* debug_span_b() const { return lex->span_b; }

  const Lexeme* get_lex_debug() const {
    return lex;
  }

  //----------------------------------------------------------------------------

private:
  ParseNode* span;
  const Lexeme* lex;
};

//------------------------------------------------------------------------------

struct ParseNode {

  //----------------------------------------

  inline static SlabAlloc slabs;

  static void* operator new(std::size_t size)   { return slabs.bump(size); }
  static void* operator new[](std::size_t size) { return slabs.bump(size); }
  static void  operator delete(void*)           { }
  static void  operator delete[](void*)         { }

  virtual ~ParseNode() {}

  //----------------------------------------

  void init(Token* tok_a, Token* tok_b) {
    constructor_count++;
    DCHECK(tok_a <= tok_b);

    this->_tok_a = tok_a;
    this->_tok_b = tok_b;

    // Attach all the tops under this node to it.
    auto cursor = tok_a;
    while (cursor <= tok_b) {
      auto child = cursor->get_span();
      if (child) {
        attach_child(child);
        cursor = child->tok_b() + 1;
      }
      else {
        cursor++;
      }
    }

    tok_a->set_span(this);
    tok_b->set_span(this);
  }

  //----------------------------------------

  void attach_child(ParseNode* child) {
    DCHECK(child);
    DCHECK(child->prev == nullptr);
    DCHECK(child->next == nullptr);

    if (tail) {
      tail->next = child;
      child->prev = tail;
      tail = child;
    }
    else {
      head = child;
      tail = child;
    }
  }

  //----------------------------------------

  int node_count() const {
    int accum = 1;
    for (auto c = head; c; c = c->next) {
      accum += c->node_count();
    }
    return accum;
  }

  //----------------------------------------

  ParseNode* left_neighbor() {
    if (this == nullptr) return nullptr;
    return (_tok_a - 1)->get_span();
  }

  ParseNode* right_neighbor() {
    if (this == nullptr) return nullptr;
    return (_tok_b + 1)->get_span();
  }

  //----------------------------------------

  template<typename P>
  bool is_a() const {
    if (this == nullptr) return false;
    return typeid(*this) == typeid(P);
  }

  template<typename P>
  P* child() {
    if (this == nullptr) return nullptr;
    for (auto cursor = head; cursor; cursor = cursor->next) {
      if (cursor->is_a<P>()) {
        return dynamic_cast<P*>(cursor);
      }
    }
    return nullptr;
  }

  template<typename P>
  const P* child() const {
    if (this == nullptr) return nullptr;
    for (auto cursor = head; cursor; cursor = cursor->next) {
      if (cursor->is_a<P>()) {
        return dynamic_cast<const P*>(cursor);
      }
    }
    return nullptr;
  }

  template<typename P>
  P* search() {
    if (this == nullptr) return nullptr;
    if (this->is_a<P>()) return this->as_a<P>();
    for (auto cursor = head; cursor; cursor = cursor->next) {
      if (auto c = cursor->search<P>()) {
        return c;
      }
    }
    return nullptr;
  }

  template<typename P>
  P* as_a() {
    return dynamic_cast<P*>(this);
  };

  template<typename P>
  const P* as_a() const {
    return dynamic_cast<const P*>(this);
  };

  void print_class_name(int max_len = 0) const {
    const char* name = typeid(*this).name();
    int name_len = 0;
    if (sscanf(name, "%d", &name_len)) {
      while((*name >= '0') && (*name <= '9')) name++;
    }
    if (name_len > max_len) name_len = max_len;

    for (int i = 0; i < name_len; i++) {
      putc(name[i], stdout);
    }
    for (int i = name_len; i < max_len; i++) {
      putc(' ', stdout);
    }
  }

  //----------------------------------------

  void check_sanity() {
    // All our children should be sane.
    for (auto cursor = head; cursor; cursor = cursor->next) {
      cursor->check_sanity();
    }

    // Our prev/next pointers should be hooked up correctly
    CHECK(!next || next->prev == this);
    CHECK(!prev || prev->next == this);

    ParseNode* cursor = nullptr;

    // Check node chain
    for (cursor = head; cursor && cursor->next; cursor = cursor->next);
    CHECK(cursor == tail);

    for (cursor = tail; cursor && cursor->prev; cursor = cursor->prev);
    CHECK(cursor == head);
  }

  //----------------------------------------

  Token* tok_a() { return _tok_a; }
  Token* tok_b() { return _tok_b; }
  const Token* tok_a() const { return _tok_a; }
  const Token* tok_b() const { return _tok_b; }

  //----------------------------------------

  inline static int constructor_count = 0;

  int precedence = 0;
  int assoc = 0;

  ParseNode* prev = nullptr;
  ParseNode* next = nullptr;
  ParseNode* head = nullptr;
  ParseNode* tail = nullptr;

private:

  Token* _tok_a = nullptr; // First token, inclusivve
  Token* _tok_b = nullptr; // Last token, inclusive
};

//------------------------------------------------------------------------------
// Consumes spans from all tokens it matches with and creates a new node on top of them.

template<typename NodeType>
struct NodeMaker : public ParseNode {
  static Token* match(void* ctx, Token* a, Token* b) {

    print_trace_start<NodeType, Token>(a);
    auto end = NodeType::pattern::match(ctx, a, b);
    print_trace_end<NodeType, Token>(a, end);

    if (end && end != a) {
      auto node = new NodeType();
      node->init(a, end-1);
    }
    return end;
  }
};

//----------------------------------------------------------------------------

inline void Token::dump_token() const {
  // Dump token
  printf("tok @ %p :", this);

  printf(" %14.14s ", type_to_str());
  set_color(type_to_color());
  lex->dump_lexeme();
  set_color(0);

  printf("    span %14p ", span);
  if (span) {
    printf("{");
    span->print_class_name(20);
    printf("}");
  }
  else {
    printf("{                    }");
  }
  printf("\n");
}

//------------------------------------------------------------------------------

inline int atom_cmp(Token& a, const LexemeType& b) {
  return int(a.get_type()) - int(b);
}

inline int atom_cmp(Token& a, const char& b) {
  int len_cmp = a.get_len() - 1;
  if (len_cmp != 0) return len_cmp;
  return int(a.as_str()[0]) - int(b);
}

template<int N>
inline int atom_cmp(Token& a, const StringParam<N>& b) {
  // FIXME do we really need this comparison?
  int len_cmp = int(a.get_len()) - int(b.str_len);
  if (len_cmp != 0) return len_cmp;

  for (auto i = 0; i < b.str_len; i++) {
    int cmp = int(a.as_str()[i]) - int(b.str_val[i]);
    if (cmp) return cmp;
  }

  return 0;
}

//------------------------------------------------------------------------------

inline Token* match_punct(void* ctx, Token* a, Token* b, const char* lit, int lit_len) {
  if (!a || a == b) return nullptr;
  if (a + lit_len > b) return nullptr;
  for (auto i = 0; i < lit_len; i++) {
    if (a->as_str()[i] != lit[i]) return nullptr;
  }
  auto end = a + lit_len;
  return end;
}

//------------------------------------------------------------------------------

struct ParseNodeIterator {
  ParseNodeIterator(ParseNode* cursor) : n(cursor) {}
  ParseNodeIterator& operator++() { n = n->next; return *this; }
  bool operator!=(ParseNodeIterator& b) const { return n != b.n; }
  ParseNode* operator*() const { return n; }
  ParseNode* n;
};

inline ParseNodeIterator begin(ParseNode* parent) {
  return ParseNodeIterator(parent->head);
}

inline ParseNodeIterator end(ParseNode* parent) {
  return ParseNodeIterator(nullptr);
}

struct ConstParseNodeIterator {
  ConstParseNodeIterator(const ParseNode* cursor) : n(cursor) {}
  ConstParseNodeIterator& operator++() { n = n->next; return *this; }
  bool operator!=(const ConstParseNodeIterator& b) const { return n != b.n; }
  const ParseNode* operator*() const { return n; }
  const ParseNode* n;
};

inline ConstParseNodeIterator begin(const ParseNode* parent) {
  return ConstParseNodeIterator(parent->head);
}

inline ConstParseNodeIterator end(const ParseNode* parent) {
  return ConstParseNodeIterator(nullptr);
}

//------------------------------------------------------------------------------

/*
inline Token* find_matching_delim(void* ctx, char ldelim, char rdelim, Token* a, Token* b) {
  if (a->as_str()[0] != ldelim) return nullptr;
  a++;

  while(a && a < b) {
    if (a->is_punct(rdelim)) return a;

    // Note that we _don't_ recurse through <> because they're not guaranteed
    // to be delimiters. Annoying aspect of C. :/

    if (a && a->is_punct('(')) a = find_matching_delim(ctx, '(', ')', a, b);
    if (a && a->is_punct('{')) a = find_matching_delim(ctx, '{', '}', a, b);
    if (a && a->is_punct('[')) a = find_matching_delim(ctx, '[', ']', a, b);

    if (!a) return nullptr;
    a++;
  }

  return nullptr;
}
*/

//------------------------------------------------------------------------------
// The Delimited<> modifier constrains a matcher to fit exactly between a pair
// of matching delimiters.
// For example, Delimited<'(', ')', NodeConstant> will match "(1)" but not
// "(1 + 2)".

/*
template<char ldelim, char rdelim, typename P>
struct Delimited {
  static Token* match(void* ctx, Token* a, Token* b) {
    if (!a || !a->is_punct(ldelim)) return nullptr;
    auto new_b = find_matching_delim(ctx, ldelim, rdelim, a, b);
    if (!new_b || !new_b->is_punct(rdelim)) return nullptr;

    if (!new_b) return nullptr;
    if (auto end = P::match(ctx, a + 1, new_b)) {
      if (end != new_b) return nullptr;
      return new_b + 1;
    }
    else {
      return nullptr;
    }
  }
};
*/

//------------------------------------------------------------------------------
/*
struct NodeDispenser {

  NodeDispenser(ParseNode** children, size_t child_count) {
    this->children = children;
    this->child_count = child_count;
  }

  template<typename P>
  operator P*() {
    if (child_count == 0) return nullptr;
    if (children[0] == nullptr) return nullptr;
    if (children[0]->is_a<P>()) {
      P* result = children[0]->as_a<P>();
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
*/
