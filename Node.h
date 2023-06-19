#pragma once
#include <assert.h>
#include "c_lexer.h"
#include <stdio.h>
#include <string>
#include <typeinfo>
#include <set>
#include <string>

#include "Tokens.h"

typedef std::set<std::string> IdentifierSet;

void set_color(uint32_t c);

//------------------------------------------------------------------------------

struct NodeBase;

struct NodeStack {
  void push(NodeBase* n);
  NodeBase* pop();
  NodeBase* back();
  size_t top() const;
  void dump_top();
  void clear_to(size_t new_top);
  void pop_to(size_t new_top);

  static const int stack_size = 8192;
  NodeBase*  _stack[stack_size] = {0};
  size_t _top = 0;
};

//------------------------------------------------------------------------------

struct NodeBase {

  void init(const Token* a = nullptr, const Token* b = nullptr) {
    //print_class_name();
    //printf("::init()\n");

    this->tok_a = a;
    this->tok_b = b;
  }

  void init(const Token* a, const Token* b, NodeBase** children, size_t child_count) {
    //print_class_name();
    //printf("::init()\n");

    this->tok_a = a;
    this->tok_b = b;

    for (auto i = 0; i < child_count; i++) {
      append(children[i]);
    }
  }

  virtual ~NodeBase() {
    auto cursor = head;
    while(cursor) {
      auto next = cursor->next;
      delete cursor;
      cursor = next;
    }
  }

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
    if (this == nullptr) return nullptr;
    if (typeid(*this) == typeid(P)) {
      return dynamic_cast<P*>(this);
    }
    else {
      assert(false);
      return nullptr;
    }
  };

  //----------------------------------------

  NodeBase* child_at(int index) {
    auto cursor = head;
    for (auto i = 0; i < index; i++) {
      if (cursor) cursor = cursor->next;
    }
    return cursor;
  }

  //----------------------------------------

  void append(NodeBase* node) {
    if (!node) return;

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

    NodeBase* cursor = nullptr;

    // Check node chain
    for (cursor = head; cursor && cursor->next; cursor = cursor->next);
    assert(cursor == tail);

    for (cursor = tail; cursor && cursor->prev; cursor = cursor->prev);
    assert(cursor == head);
  }

  //----------------------------------------

  std::string escape_span() {
    if (!tok_a || !tok_b) {
      return "<bad span>";
    }

    if (tok_a == tok_b) {
      return "<zero span>";
    }

    auto lex_a = tok_a->lex;
    auto lex_b = (tok_b - 1)->lex;
    auto len = lex_b->span_b - lex_a->span_a;

    std::string result;
    for (auto i = 0; i < len; i++) {
      auto c = lex_a->span_a[i];
      if (c == '\n') {
        result.push_back('\\');
        result.push_back('n');
      }
      else if (c == '\r') {
        result.push_back('\\');
        result.push_back('r');
      }
      else if (c == '\t') {
        result.push_back('\\');
        result.push_back('t');
      }
      else {
        result.push_back(c);
      }
      if (result.size() >= 80) break;
    }

    return result;
  }

  //----------------------------------------

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

  virtual void dump_tree(int max_depth = 0, int indentation = 0) {
    if (max_depth && indentation == max_depth) return;

    for (int i = 0; i < indentation; i++) printf(" | ");

    if (tok_a) set_color(tok_a->lex->color());
    //if (!field.empty()) printf("%-10.10s : ", field.c_str());

    print_class_name();

    printf(" '%s'\n", escape_span().c_str());

    if (tok_a) set_color(0);

    for (auto c = head; c; c = c->next) {
      c->dump_tree(max_depth, indentation + 1);
    }
  }

  //----------------------------------------

  const Token* tok_a = nullptr;
  const Token* tok_b = nullptr;

  //std::string field = "";

  NodeBase* parent = nullptr;
  NodeBase* prev   = nullptr;
  NodeBase* next   = nullptr;
  NodeBase* head   = nullptr;
  NodeBase* tail   = nullptr;

  static NodeStack node_stack;

  //static inline IdentifierSet global_ids = {};

  static inline IdentifierSet builtin_types = {
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

  static inline IdentifierSet builtin_type_prefixes = {
    "signed", "unsigned", "short", "long", "_Complex",
    "__signed__", "__unsigned__",
    "__complex__", "__real__", "__imag__",
  };

  static inline IdentifierSet builtin_type_suffixes = {
    // Why, GCC, why?
    "_Complex", "__complex__",
  };

  static inline IdentifierSet class_types = {
  };

  static inline IdentifierSet struct_types = {
  };

  static inline IdentifierSet union_types = {
  };

  static inline IdentifierSet enum_types = {
  };

  static inline IdentifierSet typedef_types = {
  };

  static void reset_types() {
    class_types.clear();
    struct_types.clear();
    union_types.clear();
    enum_types.clear();
    typedef_types.clear();
  }
};

//------------------------------------------------------------------------------

template<typename NT>
struct NodeMaker : public NodeBase {

  static const Token* match(const Token* a, const Token* b) {
    auto old_top = node_stack.top();
    auto end = NT::pattern::match(a, b);
    auto new_top = node_stack.top();

    if (!end) {
      node_stack.clear_to(old_top);
      return nullptr;
    }

    auto node = new NT();
    node->init(a, end, &node_stack._stack[old_top], new_top - old_top);
    node_stack.pop_to(old_top);
    node_stack.push(node);
    return end;
  }
};

//------------------------------------------------------------------------------

template<typename P>
struct CleanDeadNodes {
  static const Token* match(const Token* a, const Token* b) {
    auto old_top = NodeBase::node_stack.top();
    auto end = P::match(a, b);
    if (!end) {
      NodeBase::node_stack.clear_to(old_top);
    }
    return end;
  }
};

//------------------------------------------------------------------------------

template<typename NT>
struct PatternWrapper {
  static const Token* match(const Token* a, const Token* b) {
    return NT::pattern::match(a, b);
  }
};

//------------------------------------------------------------------------------

template<StringParam lit>
struct NodeOperator : public NodeMaker<NodeOperator<lit>> {
  using pattern = Operator<lit>;
};

//------------------------------------------------------------------------------

template<StringParam lit>
struct NodeKeyword : public NodeMaker<NodeKeyword<lit>> {
  using pattern = Keyword<lit>;
};

//------------------------------------------------------------------------------

struct NodeDispenser {

  NodeDispenser(NodeBase** children, size_t child_count) {
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

  NodeBase** children;
  size_t child_count;
};

//------------------------------------------------------------------------------

struct NodePreproc : public NodeMaker<NodePreproc> {
  using pattern = Atom<TOK_PREPROC>;
};

//------------------------------------------------------------------------------

struct NodeIdentifier : public NodeMaker<NodeIdentifier> {
  using pattern = Atom<TOK_IDENTIFIER>;
};

//------------------------------------------------------------------------------

struct NodeString : public NodeMaker<NodeString> {
  using pattern = Atom<TOK_STRING>;
};

//------------------------------------------------------------------------------

struct NodeConstant : public NodeMaker<NodeConstant> {
  using pattern = Oneof<
    Atom<TOK_FLOAT>,
    Atom<TOK_INT>,
    Atom<TOK_CHAR>,
    Atom<TOK_STRING>
  >;
};

//------------------------------------------------------------------------------

struct NodeBuiltinTypeBase {
  static const Token* match(const Token* a, const Token* b) {
    if (!a || !a->is_identifier() || a == b) return nullptr;

    if (NodeBase::builtin_types.contains(a->lex->text())) {
      return a + 1;
    }
    else {
      return nullptr;
    }
  }
};

struct NodeBuiltinTypePrefix {
  static const Token* match(const Token* a, const Token* b) {
    if (!a || !a->is_identifier() || a == b) return nullptr;

    if (NodeBase::builtin_type_prefixes.contains(a->lex->text())) {
      return a + 1;
    }
    else {
      return nullptr;
    }
  }
};

struct NodeBuiltinTypeSuffix {
  static const Token* match(const Token* a, const Token* b) {
    if (!a || !a->is_identifier() || a == b) return nullptr;

    if (NodeBase::builtin_type_suffixes.contains(a->lex->text())) {
      return a + 1;
    }
    else {
      return nullptr;
    }
  }
};

//------------------------------------------------------------------------------
// Our builtin types are any sequence of prefixes followed by a builtin type

struct NodeBuiltinType : public NodeMaker<NodeBuiltinType> {
  using pattern = Seq<
    Any<
      Seq<NodeBuiltinTypePrefix, And<NodeBuiltinTypeBase>>
    >,
    NodeBuiltinTypeBase,
    Opt<NodeBuiltinTypeSuffix>
  >;
};

struct NodeClassType : public NodeMaker<NodeClassType> {
  static const Token* match(const Token* a, const Token* b) {
    if (a && NodeBase::class_types.contains(a->lex->text())) return a + 1;
    return nullptr;
  }
};

struct NodeStructType : public NodeMaker<NodeStructType> {
  static const Token* match(const Token* a, const Token* b) {
    if (a && NodeBase::struct_types.contains(a->lex->text())) return a + 1;
    return nullptr;
  }
};

struct NodeUnionType : public NodeMaker<NodeUnionType> {
  static const Token* match(const Token* a, const Token* b) {
    if (a && NodeBase::union_types.contains(a->lex->text())) return a + 1;
    return nullptr;
  }
};

struct NodeEnumType : public NodeMaker<NodeEnumType> {
  static const Token* match(const Token* a, const Token* b) {
    if (a && NodeBase::enum_types.contains(a->lex->text())) return a + 1;
    return nullptr;
  }
};

struct NodeTypedefType : public PatternWrapper<NodeTypedefType> {
  static const Token* match(const Token* a, const Token* b) {
    if (a && NodeBase::typedef_types.contains(a->lex->text())) return a + 1;
    return nullptr;
  }
};

//------------------------------------------------------------------------------
