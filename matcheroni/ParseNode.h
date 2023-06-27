
#pragma once
#include <typeinfo>

#include "Lexemes.h"
#include "SlabAlloc.h"

struct Lexeme;
struct Token;
struct ParseNode;

void dump_lexeme(const Lexeme& l);
void dump_token(const Token& t);
void dump_tree(const ParseNode* n, int max_depth, int indentation);
void set_color(uint32_t c);

//------------------------------------------------------------------------------
// Tokens associate lexemes with parse nodes.
// Tokens store bookkeeping data during parsing.

struct Token {
  Token(const Lexeme* lex) {
    this->lex  = lex;
    this->base = nullptr;
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
  ParseNode* base;
  ParseNode* span;
  void* pad; // make size 32 for convenience
};

using token_matcher = matcher_function<Token>;

//------------------------------------------------------------------------------

struct ParseNode {

  //----------------------------------------

  inline static SlabAlloc slabs;

  static void* operator new(std::size_t size)   { return slabs.bump(size); }
  static void* operator new[](std::size_t size) { return slabs.bump(size); }
  static void  operator delete(void*)           { }
  static void  operator delete[](void*)         { }

  //----------------------------------------

  void init_base(Token* tok_a, Token* tok_b) {
    constructor_count++;
    assert(tok_a <= tok_b);

    for (auto c = tok_a; c <= tok_b; c++) {
      c->base = this;
      c->span = nullptr;
    }

    this->tok_a = tok_a;
    this->tok_b = tok_b;
  }

  //----------------------------------------

  void init_span(Token* tok_a, Token* tok_b) {
    constructor_count++;
    assert(tok_a <= tok_b);

    // Check that the token range is solidly filled with parse nodes
    for (auto c = tok_a; c <= tok_b; c++) {
      assert(c->base);
    }

    {
      auto c = tok_a;
      while(1) {
        assert(c <= tok_b);

        if (c->span) {
          c = c->span->tok_b;
        }
        else if (c->base) {
          c = c->base->tok_b;
        }
        else {
          assert(false);
        }

        if (c == tok_b) break;
      }
    }

    this->tok_a = tok_a;
    this->tok_b = tok_b;

    // Attach all the tops under this node to it.
    auto cursor = tok_a;
    while (cursor <= tok_b) {
      auto child = cursor->span ? cursor->span : cursor->base;
      attach_child(child);
      cursor = child->tok_b + 1;
    }

    tok_a->span = this;
    tok_b->span = this;
  }

  //----------------------------------------

  void attach_child(ParseNode* child) {
    assert(child);
    assert(child->prev == nullptr);
    assert(child->next == nullptr);

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

  ParseNode* left_span() {
    if (this == nullptr) return nullptr;
    return (tok_a - 1)->span;
  }

  ParseNode* right_span() {
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

  Token* tok_a = nullptr; // First token, inclusivve
  Token* tok_b = nullptr; // Last token, inclusive
  int precedence = 0;

  ParseNode* prev   = nullptr;
  ParseNode* next   = nullptr;
  ParseNode* head   = nullptr;
  ParseNode* tail   = nullptr;
};

//------------------------------------------------------------------------------

inline void dump_lexeme(const Lexeme& l) {
  int len = l.span_b - l.span_a;
  for (int i = 0; i < len; i++) {
    auto c = l.span_a[i];
    if (c == '\n' || c == '\t' || c == '\r') {
      putc('@', stdout);
    }
    else {
      putc(l.span_a[i], stdout);
    }
  }
}

inline void dump_token(const Token& t) {
  // Dump token
  printf("tok %p - ", &t);
  if (t.lex->is_eof()) {
    printf("<eof>");
  }
  else {
    dump_lexeme(*t.lex);
  }

  // Dump top node
  printf("  ");
  if (t.span) {
    printf("top %p -> ", t.span);
    t.span->print_class_name();
    printf(" -> %p ", t.span->tok_b);
  }
  else {
    printf("top %p", t.span);
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
  if (typeid(*(a->top)) == typeid(NT)) {
    return a->top->tok_b;
  }
}
// No node. Create a new node if the pattern matches, bail if it doesn't.
*/

//------------------------------------------------------------------------------

struct NodeBase : public ParseNode {
  template<typename P, typename NT>
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = P::match(ctx, a, b);
    if (end && end != a) {
      auto node = new NT();
      node->init_base(a, end-1);
    }
    return end;
  }
};

struct NodeSpan : public ParseNode {
  template<typename P, typename NT>
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = P::match(ctx, a, b);
    if (end && end != a) {
      auto node = new NT();
      node->init_span(a, end-1);
    }
    return end;
  }
};

template<typename NT>
struct NodeBaseMaker : public NodeBase {
  using match = NodeBase::match<NT::pattern, NT>;
};

template<typename NT>
struct NodeSpanMaker : public NodeSpan {
  using match = NodeSpan::match<NT::pattern, NT>;
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

template <auto... rest>
struct NodeAtom : public NodeBaseMaker<NodeAtom<rest...>> {
  using pattern = Atom<rest...>;
};

template<StringParam lit>
struct NodeKeyword : public NodeBaseMaker<NodeKeyword<lit>> {
  using pattern = Keyword<lit>;
};

template<StringParam lit>
struct NodePunctuator : public NodeBaseMaker<NodePunctuator<lit>> {
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = match_punct(ctx, a, b, lit.str_val, lit.str_len);
    if (end && end != a) {
      auto node = new NodePunctuator<lit>();
      node->init_base(a, end - 1);
    }
    return end;
  }
};

struct NodePreproc : public NodeBaseMaker<NodePreproc> {
  using pattern = Atom<LEX_PREPROC>;
};

struct NodeIdentifier : public NodeBaseMaker<NodeIdentifier> {
  using pattern = Atom<LEX_IDENTIFIER>;

  // used by "add_*_type"
  std::string text() const {
    return tok_a->lex->text();
  }
};

struct NodeString : public NodeBaseMaker<NodeString> {
  using pattern = Some<Atom<LEX_STRING>>;
};

struct NodeConstant : public NodeBaseMaker<NodeConstant> {
  using pattern = Oneof<
    Atom<LEX_FLOAT>,
    Atom<LEX_INT>,
    Atom<LEX_CHAR>,
    Some<Atom<LEX_STRING>>
  >;
};

// Our builtin types are any sequence of prefixes followed by a builtin type

struct NodeBuiltinType : public NodeBaseMaker<NodeBuiltinType> {
  using match_base   = Ref<&C99Parser::match_builtin_type_base>;
  using match_prefix = Ref<&C99Parser::match_builtin_type_prefix>;
  using match_suffix = Ref<&C99Parser::match_builtin_type_suffix>;

  using pattern = Seq<
    Any<Seq<match_prefix, And<match_base>>>,
    match_base,
    Opt<match_suffix>
  >;
};

template<StringParam lit>
struct NodeOpPrefix : public NodeBase {
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = match_punct(ctx, a, b, lit.str_val, lit.str_len);
    if (end) {
      auto node = new NodeOpPrefix<lit>();
      node->precedence = prefix_precedence(lit.str_val);
      node->init(a, end - 1);
    }
    return end;
  }
};

template<StringParam lit>
struct NodeOpBinary : public NodeBase {
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = match_punct(ctx, a, b, lit.str_val, lit.str_len);
    if (end) {
      auto node = new NodeOpBinary<lit>();
      node->precedence = binary_precedence(lit.str_val);
      node->init(a, end - 1);
    }
    return end;
  }
};

template<StringParam lit>
struct NodeOpSuffix : public NodeBase {
  static Token* match(void* ctx, Token* a, Token* b) {
    auto end = match_punct(ctx, a, b, lit.str_val, lit.str_len);
    if (end) {
      auto node = new NodeOpSuffix<lit>();
      node->precedence = suffix_precedence(lit.str_val);
      node->init(a, end - 1);
    }
    return end;
  }
};

template<typename P>
using comma_separated = Seq<P, Any<Seq<NodeAtom<','>, P>>, Opt<NodeAtom<','>> >;

template<typename P>
using opt_comma_separated = Opt<comma_separated<P>>;

//------------------------------------------------------------------------------
