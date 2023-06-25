
#pragma once
#include <typeinfo>
#include <set>
#include <string>
#include <vector>
#include <string.h>

#include "Lexemes.h"

struct ParseNode;

//------------------------------------------------------------------------------
// Tokens associate lexemes with parse nodes.
// Tokens store bookkeeping data during parsing.

struct Token {
  Token(const Lexeme* lex) {
    this->lex = lex;
    this->top = nullptr;
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

  const Lexeme* lex;
  ParseNode* top;
};

using token_matcher = matcher_function<Token>;

//------------------------------------------------------------------------------

struct SlabAllocator {

  SlabAllocator() {
    top_slab = new uint8_t[slab_size];
    slab_cursor = 0;
    current_size = 0;
    max_size = 0;
  }

  void reset() {
    for (auto s : old_slabs) delete [] s;
    old_slabs.clear();
    slab_cursor = 0;
    current_size = 0;
  }

  void* bump(size_t size) {
    if (slab_cursor + size > slab_size) {
      old_slabs.push_back(top_slab);
      top_slab = new uint8_t[slab_size];
      slab_cursor = 0;
    }

    auto result = top_slab + slab_cursor;
    slab_cursor += size;

    current_size += size;
    if (current_size > max_size) max_size = current_size;

    return result;
  }

  static constexpr size_t slab_size = 16*1024*1024;
  std::vector<uint8_t*> old_slabs;
  uint8_t* top_slab;
  size_t   slab_cursor;
  size_t   current_size;
  size_t   max_size;
};

//------------------------------------------------------------------------------

struct ParseNode {

  //----------------------------------------

  inline static SlabAllocator slabs;

  static void* operator new(std::size_t size)   { return slabs.bump(size); }
  static void* operator new[](std::size_t size) { return slabs.bump(size); }
  static void  operator delete(void*)           { }
  static void  operator delete[](void*)         { }

  //----------------------------------------

  void init_fixme(Token* tok_a, Token* tok_b) {
    assert(tok_a <= tok_b);

    this->tok_a = tok_a;
    this->tok_b = tok_b;
    constructor_count++;

    // Attach all the tops under this node to it.
    auto cursor = tok_a;
    while (cursor <= tok_b) {
      if (cursor->top) {
        auto child = cursor->top;
        //cursor->top = nullptr;

        if (tail) {
          tail->next = child;
          child->prev = tail;
          tail = child;
        }
        else {
          head = child;
          tail = child;
        }

        cursor = child->tok_b + 1;
      }
      else {
        cursor++;
      }
    }

    tok_a->top = this;
  }

  virtual ~ParseNode() {
    destructor_count++;
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

  inline static int constructor_count = 0;
  inline static int destructor_count = 0;

  Token* tok_a = nullptr;
  Token* tok_b = nullptr;

  ParseNode* prev   = nullptr;
  ParseNode* next   = nullptr;
  ParseNode* head   = nullptr;
  ParseNode* tail   = nullptr;
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

// The optimization below does not actually improve performance in this
// parser, though it could be significant in other ones.

// Parseroni C parser without:
// Total time     27887.913728 msec
// Constructor 565766993

// Parseroni C parser with:
// Total time     28685.710080 msec
// Constructor 559067842

// See if there's a node on the token that we can reuse
/*
if (a->top) {
  if (typeid(*(a->top)) == typeid(NT)) {
    return a->top->tok_b;
  }
}
// No node. Create a new node if the pattern matches, bail if it doesn't.
*/

template<typename NT>
struct NodeMaker : public ParseNode {

  static Token* match(void* ctx, Token* a, Token* b) {
    if (!a || a == b) return nullptr;
    auto end = NT::pattern::match(ctx, a, b);
    if (end && end != a) {
      auto node = new NT();
      node->init_fixme(a, end-1);
    }
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

inline Token* find_matching_delim(void* ctx, char ldelim, char rdelim, Token* a, Token* b) {
  if (*a->lex->span_a != ldelim) return nullptr;
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

//------------------------------------------------------------------------------
// The Delimited<> modifier constrains a matcher to fit exactly between a pair
// of matching delimiters.
// For example, Delimited<'(', ')', NodeConstant> will match "(1)" but not
// "(1 + 2)".

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
