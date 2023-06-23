
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
#include <vector>
#include <string.h>

#include "Lexemes.h"

struct ParseNode;
void set_color(uint32_t c);

void dump_tree(ParseNode* n, int max_depth = 0, int indentation = 0);

//------------------------------------------------------------------------------
// Tokens associate lexemes with parse nodes.
// Tokens store bookkeeping data during parsing.

struct Token {
  Token(const Lexeme* lex) {
    this->lex = lex;
    this->top = nullptr;
    //this->alt = nullptr;
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
  //ParseNode* alt;
};

using token_matcher = matcher_function<Token>;

//------------------------------------------------------------------------------

struct ParseNode {

#if 1
  static constexpr size_t slab_size = 16*1024*1024;
  inline static std::vector<uint8_t*> slabs;
  inline static size_t slab_cursor;
  inline static size_t current_size = 0;
  inline static size_t max_size = 0;

  static void* bump(size_t size) {
    auto slab = slabs.empty() ? nullptr : slabs.back();
    if (!slab || (slab_cursor + size > slab_size)) {
      slab = new uint8_t[slab_size];
      slab_cursor = 0;
      slabs.push_back(slab);
    }

    auto result = slab + slab_cursor;
    slab_cursor += size;

    current_size += size;
    if (current_size > max_size) max_size = current_size;

    return result;
  }

  static void clear_slabs() {
    for (auto s : slabs) delete [] s;
    slabs.clear();
    current_size = 0;
  }

  void* operator new(std::size_t size)   {
    return bump(size);
  }
  void* operator new[](std::size_t size) { return bump(size); }
  void  operator delete(void*)   {}
  void  operator delete[](void*) {}
#else
  static void clear_slabs() {}
#endif

  //uint32_t sentinel = 12345678;

  void init(Token* tok_a, Token* tok_b) {
    //this->matcher = matcher;
    this->tok_a = tok_a;
    this->tok_b = tok_b;
    //this->alt = nullptr;
    //instance_count++;
    constructor_count++;
  }


  virtual ~ParseNode() {
    //assert(sentinel == 12345678);
    //sentinel = 77777777;
    //printf("Deleting parsenode\n");

    //instance_count--;
    destructor_count++;

    /*
    auto cursor = head;
    while(cursor) {
      auto next = cursor->next;
      delete cursor;
      cursor = next;
    }
    */
  }

  //----------------------------------------

  /*
  ParseNode* top() {
    if (parent) return parent->top();
    return this;
  }
  */

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

  void detach_children() {
    auto c = head;
    while(c) {
      auto next = c->next;
      c->next = nullptr;
      c->prev = nullptr;
      //c->parent = nullptr;
      c = next;
    }
    head = nullptr;
    tail = nullptr;
  }

  //----------------------------------------
  // Attach all the tops under this node to it.

  //inline static int step_over_count = 0;

  void attach_children() {

    auto cursor = tok_a;
    while (cursor < tok_b) {
      if (cursor->top) {
        auto child = cursor->top;
        cursor->top = nullptr;

        //assert(!child->parent);
        //if (child->parent) child->parent->detach_children();

        //child->parent = this;
        if (tail) {
          tail->next = child;
          child->prev = tail;
          tail = child;
        }
        else {
          head = child;
          tail = child;
        }

        cursor = child->tok_b;
      }
      else {
        //step_over_count++;
        cursor++;
      }
    }
  }

  //----------------------------------------

  /*
  void reattach_children() {
    // Run our matcher to move all our children to the top
    auto end = matcher(tok_a, tok_b);
    assert(end == tok_b);
    attach_children();
  }
  */

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
  //inline static int instance_count = 0;

  //token_matcher matcher = nullptr;
  Token* tok_a = nullptr;
  Token* tok_b = nullptr;

  //ParseNode* parent = nullptr;
  ParseNode* prev   = nullptr;
  ParseNode* next   = nullptr;
  ParseNode* head   = nullptr;
  ParseNode* tail   = nullptr;
  //ParseNode* alt    = nullptr;

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

//#define USE_MEMO
//#define MEMOIZE_UNMATCHES

struct NodeArraySuffix;

template<typename NT>
struct NodeMaker : public ParseNode {

  static Token* match(Token* a, Token* b) {

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
    */


    NT* node = nullptr;

    // No node. Create a new node if the pattern matches, bail if it doesn't.
    auto end = NT::pattern::match(a, b);

    if (end) {
      node = new NT();
      //node->init(NT::pattern::match, a, end);
      node->init(a, end);
      node->attach_children();

      // And now our new node becomes token A's top.
      //node->alt = a->alt;
      //a->alt = node;
      a->top = node;
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
