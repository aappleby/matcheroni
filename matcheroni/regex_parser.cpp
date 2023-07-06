#include "Matcheroni.hpp"
#include <stdio.h>
#include <string.h>
#include <vector>

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

std::vector<Node*> node_stack;

template<>
inline void atom_rewind(void* ctx, const char* a, const char* b) {
  //printf("rewind to {%-20.20s}\n", a);
  while(node_stack.size() && node_stack.back()->b > a) {
    delete node_stack.back();
    node_stack.pop_back();
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

template<StringParam type, typename P, typename NodeType>
struct Factory {
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

//------------------------------------------------------------------------------

template<StringParam type, typename P>
using Capture = Factory<type, Trace<type, P>, Node>;
//using Capture = Factory<type, P>;

template<auto t> using RAtom    = Capture<"ratom",    Atom<t>>;
template<auto t> using NotRAtom = Capture<"notratom", NotAtom<t>>;
template<auto t> using AnyRAtom = Capture<"anyratom", AnyAtom>;

const char* match_regex(void* ctx, const char* a, const char* b);
using regex = Ref<match_regex>;

using meta      = Capture<"meta",     Seq<RAtom<'\\'>, AnyAtom>>;
using rchar     = Capture<"rchar",    Oneof<NotAtom<'\\', ')', '|', '$', '.', '+', '*', '?'>, meta>>;
using range     = Capture<"range",    Seq<rchar, RAtom<'-'>, rchar>>;
using set_item  = Capture<"set_item", Oneof<range, rchar>>;
using pos_set   = Capture<"pos_set",  Seq<RAtom<'['>, Some<set_item>, RAtom<']'>>>;
using neg_set   = Capture<"neg_set",  Seq<RAtom<'['>, RAtom<'^'>, Some<set_item>, RAtom<']'>>>;
using set       = Capture<"set",      Oneof<neg_set, pos_set>>; // Negative first, so we catch the ^!
using eos       = Capture<"eos",      RAtom<'$'>>;
using any       = Capture<"any",      RAtom<'.'>>;
using group     = Capture<"group",    Seq<RAtom<'('>, regex, RAtom<')'>>>;
using unit      = Capture<"unit",     Oneof<group, any, eos, rchar, set>>;
using star      = Capture<"star",     Seq<unit, RAtom<'*'>>>;
using plus      = Capture<"plus",     Seq<unit, RAtom<'+'>>>;
using opt       = Capture<"opt",      Seq<unit, RAtom<'?'>>>;
using simple    = Capture<"simple",   Some<star, plus, opt, unit>>;
using regex_top = Capture<"regex",    Seq< simple, Opt<Seq< RAtom<'|'>, regex >> > >;

const char* match_regex(void* ctx, const char* a, const char* b) {
  return regex_top::match(ctx, a, b);
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  printf("argv[0] = %s\n", argv[0]);
  if (argc == 2) printf("argv[1] = %s\n", argv[1]);

  const char* regex = argv[1];
  //regex = "[^a-c][b-d]";
  //regex = "a|b|c";

  auto end = regex::match(nullptr, regex, regex + + strlen(regex));

  printf("node stack size %ld\n", node_stack.size());
  printf("\n");
  printf("parse tree:\n");
  if (node_stack.size()) {
    for (auto n : node_stack) n->dump();
  }

  printf("end = %s\n", end);

}

//------------------------------------------------------------------------------
