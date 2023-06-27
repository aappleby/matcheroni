
#pragma once
#include <typeinfo>
#include <set>
#include <string>
#include <vector>
#include <string.h>

#include "Lexemes.h"

struct ParseNode;

void dump_tree(const ParseNode* n, int max_depth, int indentation);

//------------------------------------------------------------------------------
// Tokens associate lexemes with parse nodes.
// Tokens store bookkeeping data during parsing.

struct Token {
  Token(const Lexeme* lex) {
    this->lex = lex;
    this->node_r = nullptr;
    this->node_l = nullptr;
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
  ParseNode* node_r; // The parse node _starting_ at this token
  ParseNode* node_l; // The parse node _ending_ at this token
  void* pad; // make size 32 for convenience
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

  void init(Token* tok_a, Token* tok_b) {
    assert(tok_a <= tok_b);

    this->tok_a = tok_a;
    this->tok_b = tok_b;
    constructor_count++;

    // Attach all the tops under this node to it.
    auto cursor = tok_a;
    while (cursor <= tok_b) {
      if (cursor->node_r) {
        auto child = cursor->node_r;
        assert(child->tok_a == cursor);

        child->tok_a->node_r = nullptr;
        child->tok_b->node_l = nullptr;


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

    tok_a->node_r = this;
    tok_b->node_l = this;
  }

  virtual ~ParseNode() {
    destructor_count++;
  }

  //----------------------------------------

  void check_solid() const {
    if (head && head->tok_a != tok_a) {
      printf("head for %p not at tok_a\n", this);
      dump_tree(this, 0, 0);
      return;
    }
    if (tail && tail->tok_b != tok_b) {
      printf("tail for %p not at tok_b\n", this);
      dump_tree(this, 0, 0);
      return;
    }

    for (auto child = head; child; child = child->next) {
      child->check_solid();

      if (child->next) {
        if (child->tok_b + 1 != child->next->tok_a) {
          printf("child at %p not tight against its sibling\n", child);
          return;
        }
      }
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
    return (tok_a - 1)->node_l;
  }

  ParseNode* right_neighbor() {
    if (this == nullptr) return nullptr;
    return (tok_a + 1)->node_r;
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

  void print_class_name() const {
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

  Token* tok_a = nullptr; // First token, inclusivve
  Token* tok_b = nullptr; // Last token, inclusive

  ParseNode* prev   = nullptr;
  ParseNode* next   = nullptr;
  ParseNode* head   = nullptr;
  ParseNode* tail   = nullptr;
};

//------------------------------------------------------------------------------
// FIXME we could maybe change this back to "a.node_r = nullptr"

inline int atom_cmp(Token& a, const LexemeType& b) {
  if (a.node_r) {
    a.node_r->tok_b->node_l = nullptr;
    a.node_r = nullptr;
  }
  return int(a.lex->type) - int(b);
}

inline int atom_cmp(Token& a, const char& b) {
  if (a.node_r) {
    a.node_r->tok_b->node_l = nullptr;
    a.node_r = nullptr;
  }
  int len_cmp = a.lex->len() - 1;
  if (len_cmp != 0) return len_cmp;
  return int(a.lex->span_a[0]) - int(b);
}

template<int N>
inline bool atom_cmp(Token& a, const StringParam<N>& b) {
  if (a.node_r) {
    a.node_r->tok_b->node_l = nullptr;
    a.node_r = nullptr;
  }
  int len_cmp = int(a.lex->len()) - int(b.str_len);
  if (len_cmp != 0) return len_cmp;

  for (auto i = 0; i < b.str_len; i++) {
    int cmp = int(a.lex->span_a[i]) - int(b.str_val[i]);
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

void set_color(uint32_t c);

template<typename NT>
struct NodeMaker : public ParseNode {
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = NT::pattern::match(ctx, a, b);
    if (end && end != a) {
      auto node = new NT();
      node->init(a, end-1);
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

//------------------------------------------------------------------------------

/*
struct NodeKeyword : public ParseNode {
  NodeKeyword(const char* keyword) : keyword(keyword) {}
  const char* keyword;
};

template<StringParam lit>
struct MatchKeyword {
  static Token* match(void* ctx, Token* a, Token* b) {
    if (!a || a == b) return nullptr;
    if (atom_cmp(*a, lit) == 0) {
      auto end = a + 1;
      auto node = new NodeKeyword(lit.str_val);
      node->init(a, end - 1);
      return end;
    } else {
      return nullptr;
    }
  }
};
*/

template<StringParam lit>
struct NodeKeyword : public NodeMaker<NodeKeyword<lit>> {
  using pattern = Keyword<lit>;
};

template <auto... rest>
struct NodeAtom : public NodeMaker<NodeAtom<rest...>> {
  using pattern = Atom<rest...>;
};

//------------------------------------------------------------------------------
