
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
    this->lex2  = lex;
    this->span = nullptr;
  }

  LexemeType get_type() {
    const_cast<Token*>(this)->span = nullptr;
    return lex2->type;
  }

  const char* as_str() const {
    const_cast<Token*>(this)->span = nullptr;
    return lex2->span_a;
  }

  const char* span_a() const {
    const_cast<Token*>(this)->span = nullptr;
    return lex2->span_a;
  }

  const char* span_b() const {
    const_cast<Token*>(this)->span = nullptr;
    return lex2->span_b;
  }

  bool is_punct() const {
    const_cast<Token*>(this)->span = nullptr;
    return lex2->is_punct();
  }

  bool is_punct(char p) const {
    const_cast<Token*>(this)->span = nullptr;
    return lex2->is_punct(p);
  }

  int get_len() const {
    return int(span_b() - span_a());
  }

  ParseNode* get_span() {
    return span;
  }

  void set_span(ParseNode* n) {
    span = n;
  }



  void dump_token();

  //----------------------------------------------------------------------------

  const char* type_to_str() {
    switch(lex2->type) {
      case LEX_INVALID    : return "LEX_INVALID";
      case LEX_SPACE      : return "LEX_SPACE";
      case LEX_NEWLINE    : return "LEX_NEWLINE";
      case LEX_STRING     : return "LEX_STRING";
      case LEX_KEYWORD    : return "LEX_KEYWORD";
      case LEX_IDENTIFIER : return "LEX_IDENTIFIER";
      case LEX_COMMENT    : return "LEX_COMMENT";
      case LEX_PREPROC    : return "LEX_PREPROC";
      case LEX_FLOAT      : return "LEX_FLOAT";
      case LEX_INT        : return "LEX_INT";
      case LEX_PUNCT      : return "LEX_PUNCT";
      case LEX_CHAR       : return "LEX_CHAR";
      case LEX_SPLICE     : return "LEX_SPLICE";
      case LEX_FORMFEED   : return "LEX_FORMFEED";
      case LEX_BOF        : return "LEX_BOF";
      case LEX_EOF        : return "LEX_EOF";
      case LEX_LAST       : return "<lex last>";
    }
    return "<invalid>";
  }

  //----------------------------------------------------------------------------

  uint32_t type_to_color() {
    switch(lex2->type) {
      case LEX_INVALID    : return 0x0000FF;
      case LEX_SPACE      : return 0x804040;
      case LEX_NEWLINE    : return 0x404080;
      case LEX_STRING     : return 0x4488AA;
      case LEX_KEYWORD    : return 0x0088FF;
      case LEX_IDENTIFIER : return 0xCCCC40;
      case LEX_COMMENT    : return 0x66AA66;
      case LEX_PREPROC    : return 0xCC88CC;
      case LEX_FLOAT      : return 0xFF88AA;
      case LEX_INT        : return 0xFF8888;
      case LEX_PUNCT      : return 0x808080;
      case LEX_CHAR       : return 0x44DDDD;
      case LEX_SPLICE     : return 0x00CCFF;
      case LEX_FORMFEED   : return 0xFF00FF;
      case LEX_BOF        : return 0x00FF00;
      case LEX_EOF        : return 0x0000FF;
      case LEX_LAST       : return 0xFF00FF;
    }
    return 0x0000FF;
  }

  //----------------------------------------------------------------------------

private:
  ParseNode* span;
  const Lexeme* lex2;
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

  void check_solid() const {
    if (head && head->tok_a != tok_a) {
      printf("head for %p not at tok_a\n", this);
      for (auto t = tok_a; t <= tok_b; t++) t->dump_token();
      dump_tree(this, 0, 0);
      return;
    }
    if (tail && tail->tok_b != tok_b) {
      printf("tail for %p not at tok_b\n", this);
      for (auto t = tok_a; t <= tok_b; t++) t->dump_token();
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
    return (tok_a - 1)->get_span();
  }

  ParseNode* right_neighbor() {
    if (this == nullptr) return nullptr;
    return (tok_b + 1)->get_span();
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

  void sanity() {
    // All our children should be sane.
    for (auto cursor = head; cursor; cursor = cursor->next) {
      cursor->sanity();
    }

    // Our prev/next pointers should be hooked up correctly
    DCHECK(!next || next->prev == this);
    DCHECK(!prev || prev->next == this);

    ParseNode* cursor = nullptr;

    // Check node chain
    for (cursor = head; cursor && cursor->next; cursor = cursor->next);
    DCHECK(cursor == tail);

    for (cursor = tail; cursor && cursor->prev; cursor = cursor->prev);
    DCHECK(cursor == head);
  }

  //----------------------------------------

  inline static int constructor_count = 0;

  Token* tok_a = nullptr; // First token, inclusivve
  Token* tok_b = nullptr; // Last token, inclusive

  int precedence = 0;
  int assoc = 0;

  ParseNode* prev   = nullptr;
  ParseNode* next   = nullptr;
  ParseNode* head   = nullptr;
  ParseNode* tail   = nullptr;

  void init(Token* tok_a, Token* tok_b) {
    constructor_count++;
    DCHECK(tok_a <= tok_b);

    this->tok_a = tok_a;
    this->tok_b = tok_b;

    // Attach all the tops under this node to it.
    auto cursor = tok_a;
    while (cursor <= tok_b) {
      auto child = cursor->get_span();
      if (child) {
        attach_child(child);
        cursor = child->tok_b + 1;
      }
      else {
        cursor++;
      }
    }

    tok_a->set_span(this);
    tok_b->set_span(this);
  }
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

//------------------------------------------------------------------------------









//----------------------------------------------------------------------------

inline void Token::dump_token() {
  // Dump token
  printf("tok @ %p :", this);

  printf(" %14.14s ", type_to_str());
  set_color(type_to_color());
  lex2->dump_lexeme();
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
  if (typeid(*(a->top)) == typeid(NodeType)) {
    return a->top->tok_b;
  }
}
// No node. Create a new node if the pattern matches, bail if it doesn't.
*/


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
