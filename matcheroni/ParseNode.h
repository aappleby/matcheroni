
#pragma once
#include "Lexemes.h"
#include "SlabAlloc.h"
#include <typeinfo>
#include <assert.h>

struct Token;
struct ParseNode;

void dump_lexeme(const Lexeme& l);
void dump_token(const Token& t);
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
  ParseNode* span;
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
      for (auto t = tok_a; t <= tok_b; t++) dump_token(*t);
      dump_tree(this, 0, 0);
      return;
    }
    if (tail && tail->tok_b != tok_b) {
      printf("tail for %p not at tok_b\n", this);
      for (auto t = tok_a; t <= tok_b; t++) dump_token(*t);
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
    return (tok_a - 1)->span;
  }

  ParseNode* right_neighbor() {
    if (this == nullptr) return nullptr;
    return (tok_b + 1)->span;
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
};

//------------------------------------------------------------------------------
// A parse node that sits directly on the tokens with no other nodes underneath
// it.

struct NodeBase : public ParseNode {

  void init_base(Token* tok_a, Token* tok_b) {
    constructor_count++;
    DCHECK(tok_a <= tok_b);

    this->tok_a = tok_a;
    this->tok_b = tok_b;

    // Wipe spans out from under this node
    for (auto c = tok_a + 1; c < tok_b; c++) {
      c->span = nullptr;
    }

    tok_a->span = this;
    tok_b->span = this;
  }
};

//------------------------------------------------------------------------------
// A parse node that can have other parse nodes under it.

struct NodeSpan : public ParseNode {

  void init_span(Token* tok_a, Token* tok_b) {
    constructor_count++;
    DCHECK(tok_a <= tok_b);

    this->tok_a = tok_a;
    this->tok_b = tok_b;

    // Attach all the tops under this node to it.
    auto cursor = tok_a;
    while (cursor <= tok_b) {
      if (cursor->span) {
        auto child = cursor->span;
        attach_child(child);
        cursor = child->tok_b + 1;
        child->tok_a->span = nullptr;
        child->tok_b->span = nullptr;
      }
      else {
        cursor++;
      }
    }

#ifdef DEBUG
    for (auto c = tok_a + 1; c < tok_b; c++) {
      DCHECK(c->span == nullptr);
    }
#endif

    tok_a->span = this;
    tok_b->span = this;
  }
};

//------------------------------------------------------------------------------
// Flattens all tokens it matches with. Does not create a new node.

template<typename NodeType>
struct FlatMaker {
  static Token* match(void* ctx, Token* a, Token* b) {

    print_trace_start<NodeType, Token>(a);
    auto end = NodeType::pattern::match(ctx, a, b);
    print_trace_end<NodeType, Token>(a, end);

    for (auto c = a; c < end; c++) c->span = nullptr;

    return end;
  }
};

//------------------------------------------------------------------------------
// Flattens all tokens it matches with and creates a new node on top of them.

template<typename NodeType>
struct BaseMaker : public NodeBase {
  static Token* match(void* ctx, Token* a, Token* b) {

    print_trace_start<NodeType, Token>(a);
    auto end = NodeType::pattern::match(ctx, a, b);
    print_trace_end<NodeType, Token>(a, end);

    if (end && end != a) {
      auto node = new NodeType();
      node->init_base(a, end-1);
    }

    return end;
  }
};

//------------------------------------------------------------------------------
// Consumes spans from all tokens it matches with and creates a new node on top of them.

template<typename NodeType>
struct SpanMaker : public NodeSpan {
  static Token* match(void* ctx, Token* a, Token* b) {

    print_trace_start<NodeType, Token>(a);
    auto end = NodeType::pattern::match(ctx, a, b);
    print_trace_end<NodeType, Token>(a, end);

    if (end && end != a) {
      auto node = new NodeType();
      node->init_span(a, end-1);
    }
    return end;
  }
};

//------------------------------------------------------------------------------























































































//------------------------------------------------------------------------------

inline void dump_lexeme(const Lexeme& l) {
  if (l.is_eof()) {
    printf("{<eof>     }");
    return;
  }
  if (l.is_bof()) {
    printf("{<bof>     }");
    return;
  }

  int len = l.span_b - l.span_a;
  if (len > 10) len = 10;
  printf("{");
  for (int i = 0; i < len; i++) {
    auto c = l.span_a[i];
    if (c == '\n' || c == '\t' || c == '\r') {
      putc('@', stdout);
    }
    else {
      putc(l.span_a[i], stdout);
    }
  }
  for (int i = len; i < 10; i++) {
    printf(" ");
  }
  printf("}");
}

//------------------------------------------------------------------------------

inline void dump_token(const Token& t) {
  // Dump token
  printf("tok @ %p ", &t);

  printf("    lex ");
  dump_lexeme(*t.lex);

  printf("    span %14p ", t.span);
  if (t.span) {
    printf("{");
    t.span->print_class_name(20);
    printf("}");
  }
  else {
    printf("{                    }");
  }
  printf("\n");
}

//------------------------------------------------------------------------------
// FIXME we could maybe change this back to "a.node_r = nullptr"

inline int atom_cmp(Token& a, const LexemeType& b) {
  return int(a.lex->type) - int(b);
}

inline int atom_cmp(Token& a, const char& b) {
  int len_cmp = a.lex->len() - 1;
  if (len_cmp != 0) return len_cmp;
  return int(a.lex->span_a[0]) - int(b);
}

template<int N>
inline bool atom_cmp(Token& a, const StringParam<N>& b) {
  int len_cmp = int(a.lex->len()) - int(b.str_len);
  if (len_cmp != 0) return len_cmp;

  for (auto i = 0; i < b.str_len; i++) {
    int cmp = int(a.lex->span_a[i]) - int(b.str_val[i]);
    if (cmp) return cmp;
  }

  return 0;
}

inline Token* match_punct(void* ctx, Token* a, Token* b, const char* lit, int lit_len) {
  if (!a || a == b) return nullptr;
  if (a + lit_len > b) return nullptr;
  for (auto i = 0; i < lit_len; i++) {
    if (!a->lex[i].is_punct(lit[i])) return nullptr;
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

/*
FIXME how does this work with flat/base/span?
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

//------------------------------------------------------------------------------

template<auto P>
struct FlatAtom : public FlatMaker<FlatAtom<P>> {
  using pattern = Atom<P>;
};

template<StringParam lit>
struct FlatKeyword : public FlatMaker<FlatKeyword<lit>> {
  using pattern = Keyword<lit>;
};

struct FlatConstant : public FlatMaker<FlatConstant> {
  using pattern = Oneof<
    Atom<LEX_FLOAT>,
    Atom<LEX_INT>,
    Atom<LEX_CHAR>,
    Some<Atom<LEX_STRING>>
  >;
};

template<StringParam lit>
struct FlatOperator {
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = match_punct(ctx, a, b, lit.str_val, lit.str_len);
    if (end) {
      for (auto c = a; c < end; c++) c->span = nullptr;
    }
    return end;
  }
};

//------------------------------------------------------------------------------

template<auto P>
struct BaseAtom : public BaseMaker<BaseAtom<P>> {
  using pattern = Atom<P>;
};

template<StringParam lit>
struct BaseKeyword : public BaseMaker<BaseKeyword<lit>> {
  using pattern = Keyword<lit>;
};

struct BaseIdentifier : public BaseMaker<BaseIdentifier> {
  using pattern = Atom<LEX_IDENTIFIER>;
};

struct BasePreproc : public BaseMaker<BasePreproc> {
  using pattern = Atom<LEX_PREPROC>;
};

struct BaseConstant : public BaseMaker<BaseConstant> {
  using pattern = Oneof<
    Atom<LEX_FLOAT>,
    Atom<LEX_INT>,
    Atom<LEX_CHAR>,
    Some<Atom<LEX_STRING>>
  >;
};

template<StringParam lit>
struct BaseOperator : public NodeBase {
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = match_punct(ctx, a, b, lit.str_val, lit.str_len);
    if (end && end != a) {
      auto node = new BaseOperator<lit>();
      node->init_base(a, end - 1);
    }
    return end;
  }
};

//------------------------------------------------------------------------------

/*
template<typename NodeType, typename pattern>
struct Capture {
  static Token* match(void* ctx, Token* a, Token* b) {
    if (!a || a == b) return nullptr;
    auto end = pattern::match(ctx, a, b);
    if (end && end != a) {
      auto node = new NodeType();
      node->init_span(a, end - 1);
    }
    return end;
  }
};
*/

template<typename P>
using comma_separated = Seq<P, Any<Seq<Atom<','>, P>>, Opt<Atom<','>> >;

template<typename P>
using opt_comma_separated = Opt<comma_separated<P>>;

//------------------------------------------------------------------------------
