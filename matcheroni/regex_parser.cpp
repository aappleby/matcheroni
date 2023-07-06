#include "Matcheroni.hpp"
#include <stdio.h>
#include <string.h>
#include <vector>

//------------------------------------------------------------------------------

struct Node {
  const char* type;
  const char* a;
  const char* b;

  void dump(int depth = 0) {
    printf("{%-20.20s} ", a);
    for (int i = 0; i < depth; i++) printf("|  ");
    printf("%s\n", type);
    for (auto c : children) c->dump(depth+1);
  }

  std::vector<Node*> children;
};

//------------------------------------------------------------------------------

struct FactoryState {
  inline static std::vector<Node*> node_stack;
};

template<StringParam type, typename P, typename NodeType>
struct Factory : public FactoryState {
  static const char* match(void* ctx, const char* a, const char* b) {
    if (!a || a == b) return nullptr;

    auto old_top = node_stack.size();
    auto end = P::match(ctx, a, b);
    auto new_top = node_stack.size();

    if (end) {
      auto new_node = new NodeType();
      new_node->type = type.str_val;
      new_node->a = a;
      new_node->b = end;
      for (int i = old_top; i < new_top; i++) {
        new_node->children.push_back(node_stack[i]);
      }
      node_stack.resize(old_top);
      node_stack.push_back(new_node);
    }

    return end;
  }
};

template<>
inline void atom_rewind(void* ctx, const char* a, const char* b) {
  //printf("rewind to {%-20.20s}\n", a);
  while(FactoryState::node_stack.size() && FactoryState::node_stack.back()->b > a) {
    delete FactoryState::node_stack.back();
    FactoryState::node_stack.pop_back();
  }
}

//------------------------------------------------------------------------------

struct TraceState {
  inline static int depth = 0;
};

template<StringParam type, typename P>
struct Trace : public TraceState {

  static void print_bar(const char* a, const char* suffix) {
    printf("{%-20.20s} ", a);
    for (int i = 0; i < depth; i++) printf("|  ");
    printf("%s %s\n", type.str_val, suffix);
  }

  static const char* match(void* ctx, const char* a, const char* b) {
    if (!a || a == b) return nullptr;
    print_bar(a, "?");
    depth++;
    auto end = P::match(ctx, a, b);
    depth--;
    print_bar(a, end ? "OK" : "X");
    return end;
  }
};

//------------------------------------------------------------------------------

#ifdef TRACE
template<StringParam type, typename P>
using Capture = Factory<type, Trace<type, P>, Node>;
#else
template<StringParam type, typename P>
using Capture = Factory<type, P, Node>;
#endif

const char* match_regex(void* ctx, const char* a, const char* b);
using regex = Ref<match_regex>;

using meta      = Capture<"meta",      Seq<Atom<'\\'>, AnyAtom>>;
using rchar     = Capture<"rchar",     Oneof<NotAtom<'\\', '(', ')', '|', '$', '.', '+', '*', '?', '[', ']', '^'>, meta>>;
using text      = Capture<"text",      Some<rchar>>;
using range     = Capture<"range",     Seq<rchar, Atom<'-'>, rchar>>;
using set_item  = Capture<"set_item",  Oneof<range, rchar>>;
using pos_set   = Capture<"pos_set",   Seq<Atom<'['>, Some<set_item>, Atom<']'>>>;
using neg_set   = Capture<"neg_set",   Seq<Atom<'['>, Atom<'^'>, Some<set_item>, Atom<']'>>>;
using set       = Capture<"set",       Oneof<neg_set, pos_set>>; // Negative first, so we catch the ^!
using eol       = Capture<"eos",       Atom<'$'>>;
using dot       = Capture<"dot",       Atom<'.'>>;
using group     = Capture<"group",     Seq<Atom<'('>, regex, Atom<')'>>>;
using unit      =                      Oneof<group, dot, eol, text, set>;
using star      = Capture<"star",      Seq<unit, Atom<'*'>>>;
using plus      = Capture<"plus",      Seq<unit, Atom<'+'>>>;
using opt       = Capture<"opt",       Seq<unit, Atom<'?'>>>;
using simple    =                      Some<star, plus, opt, unit>;
using alternate = Capture<"alternate", Seq< simple, Some<Seq< Atom<'|'>, simple >> > >;
using regex_top = Capture<"regex",     Oneof< alternate, simple > >;

const char* match_regex(void* ctx, const char* a, const char* b) {
  return regex_top::match(ctx, a, b);
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  printf("argv[0] = %s\n", argv[0]);
  if (argc == 2) printf("argv[1] = %s\n", argv[1]);

  const char* regex = argv[1];
  //regex = "ab|bc|cd";
  auto end = regex::match(nullptr, regex, regex + + strlen(regex));

  if (FactoryState::node_stack.size()) {
    printf("Parse tree:\n");
    for (auto n : FactoryState::node_stack) n->dump();
  }
  else {
    printf("Parse failed!\n");
  }
}

//------------------------------------------------------------------------------
