# Matcheroni Documentation

[Matcheroni](https://github.com/aappleby/Matcheroni/blob/main/matcheroni/Matcheroni.hpp) is a minimalist, zero-dependency, single-header C++20 library for doing pattern matching using [Parsing Expression Grammars](https://en.wikipedia.org/wiki/Parsing_expression_grammar) (PEGs). PEGs are similar to regular expressions, but both more and less powerful.

[Parseroni](https://github.com/aappleby/Matcheroni/blob/main/matcheroni/Parseroni.hpp) is a companion single-header library that can capture the content of Matcheroni patterns and assemble them into concrete [parse trees](https://en.wikipedia.org/wiki/Parse_tree).

Matcheroni and Parseroni generate tiny, fast parsers that are easy to customize and integrate into your existing codebase.

--------------------------------------------------------------------------------
# Tutorial

[Check out the tutorial here](tutorial/) - it will walk you through building a JSON parser using Matcheroni and Parseroni.

--------------------------------------------------------------------------------
# Quick Reference
&nbsp;

### Translating from regular expressions to Matcheroni:

&nbsp;

| Regex syntax | Matcheroni syntax                   | Description |
| -----------  | ----------------------------------- | ----------- |
| a            | Atom<'a'>                     | Matches a single atom. |
| abc          | Lit<"abc">                    | Matches a literal string. |
| .            | AnyAtom<>                     | Matches any single atom. |
| \\w          | Range<'a','z','A','Z','0','9'>| Matches "word" characters. |
| [a-z]        | Range<'a','z'>                | Matches a range of atoms. |
| [a-zA-Z0-9]  | Range<'a','z','A','Z','0','9'>| Matches multiple ranges of atoms. |
| [^a-z]       | NotRange<'a','z'>             | Matches atoms _not_ in a range. |
| (X\|Y)       | Oneof<X,Y>                    | Tests patterns X & Y in order, stopping at the first match. |
| X*           | Any<X>                        | Matches zero or more instances of pattern X. |
| X+           | Some<X>                       | Matches one or more instances of pattern X. |
| X?           | Opt<X>                        | Matches zero or one instance of pattern X. |
| X{N}         | Rep<N, X>                     | Matches N repetitions of pattern X. |
| X{M,N}       | RepRange<N, M, X>             | Matches between M and N repetitions of pattern X. |

&nbsp;

### Translating from PEG rules to Matcheroni:

&nbsp;

| PEG rules         | Matcheroni syntax                   | Description |
| ----------------- | ----------------------------------- | ----------- |
| Terminal          | Atom<'a'>, Lit<"foo">   | Matches a single atom or a literal string. |
| Sequence          | Seq<X,Y>                      | Matches a sequence of sub-patterns. |
| Ordered Choice    | Oneof<X,Y>                    | Tests sub-patterns in order, stopping at the first match. |
| Zero-or-more      | Any<X>                        | Matches zero or more instances of the sub-pattern(s). |
| One-or-more       | Some<X>                       | Matches one or more instances of the sub-pattern(s). |
| Optional          | Opt<X>                        | Matches zero or one instance of the sub-pattern(s). |
| And-predicate     | And<X>                        | Fails matching if the pattern does _not_ match. Does _not_ consume any input. |
| Not-predicate     | Not<X>                        | Fails matching if the pattern _does_ match. Does _not_ consume any input. |

&nbsp;

--------------------------------------------------------------------------------
## All built-in Matcheroni patterns

Patterns that accept <...> are variadic.

Items in the Matches column formatted as "a\|b" indicate where the match would end for the concatenated string "ab"

| Name | Description | Example | Matches | Does Not Match |
|------|-------------|---------|---------|----------------|
| Atom<X> | Matches a single atom | Atom<'a'> | a | z |
| Atom<...> | Matches a single instance of one of the arguments | Atom<'a','b','c'> | a, b, c | z |
| NotAtom<X> | Matches any atom except the argument | NotAtom<'a'> | z | a |
| NotAtom<...> | Matches any atom except one of the arguments | NotAtom<'a', 'b', 'c'> | z | a, b, c |
| AnyAtom | Matches any single atom | AnyAtom | a, b, c, z |  |
| Range<X,Y> | Matches a single atom in a range of values | Range<'a','z'> | a, b, c | 3 |
| Range<...> | Matches a single atom in any of the argument ranges | Range<'a','z', 'A','Z', '0','9'> | q, 3, X | $ |
| NotRange<X,Y> | Matches a single atom not in the argument range | NotRange<'a','z'> | 3, #, ! | a, b, c |
| NotRange<...> | Matches a single atom not in any argument range | NotRange<'a','z','A','Z','0','9'> | $, ~, ! | q, 3, X |
| Lit<X> | Matches a sequence of atoms with a string literal | Lit<"true"> | true | false |
| Seq<...> | Matches a sequence of sub-patterns | Seq<Atom<'a'>, Atom<'b'>> | ab | ba |
| Oneof<...> | Tests sub-patterns in order, stopping at the first match.* | Oneof<Lit<"true">, Lit<"false">> | true, false | null |
| One<X> | Matches exactly one instance of the sub-pattern.* | One<Atom<'a'>> | a | b |
| Opt<X> | Matches zero or one instance of the sub-pattern.  | Opt<Atom<'a'>> | a, \|b, \|z | |
| Any<...> | Matches zero or more instances of any sub-pattern. | Any<Atom<'a'>, Atom<'b'>> | abbabab\|z, \|z | |
| Nothing | Matches nothing, a no-op. Used as a placeholder. | Nothing | \|a, \|b, \|c | |
| Some<X> | Matches one or more instances of the sub-pattern. | Some<Atom<'a'>> | aaaaa\|b | b |
| Some<...> | Matches one or more instances of any sub-pattern. | Some<Atom<'a'>, Atom<'b'>> | a\|c, b\|c, aaba\|c | c |
| And<X> | Matches but does not consume the sub-pattern | And<Atom<'a'>> | \|a | b |
| Not<X> | Logical inverse of And<>. | Not<Atom<'a'>> | \|b | a |
| SeqOpt<...> | Matches arguments in order, stopping at the first non-match. | SeqOpt&ltAtom<'a'>, Atom<'b'>> | \|x, a\|x, ab\|x | |
| NotEmpty<X> | Makes empty matches fail. Logical inverse of Opt<> | NotEmpty<Opt<Atom<'a'>>> | a | b |
| Rep<N,X> | Matches exactly N instances of the sub-pattern. | Rep<4, Atom<'a'>> | aaaa, aaaa\|a | aaa, aaab |
| RepRange<M,N,X> | Matches M to N instances of the sub-pattern. | RepRange<2, 3, Atom<'a'>> | aa\|b, aaa\|b, aaa\|a | ab |


 * Oneof<> returns the _first_ match, not the _longest_ match. See caveats in the tutorial.
 * One<> is basically a placeholder - it can be used to make pattern formatting more readable.

| Todo |
|------|
| Until |
| Ref |
| StoreBackref |
| MatchBackref |
| DelimitedBlock |
| DelimitedList |
| EOL |
| Charset |

- Until<x> matches anything until X matches, but does not consume X. Equivalent to Any<Seq<Not<x>,AnyAtom>>
- EOL matches newlines and end-of-file, but does not advance past it.
- Keyword<x> matches a C string literal as if it was a single atom - this is only useful if your atom type can represent whole strings.
- Charset<x> matches any char atom in the string literal x, which can be much more concise than Atom<'a', 'b', 'c', 'd', ...>

You can find some useful prewritten patterns in the [Cookbook](../matcheroni/Cookbook.hpp)

&nbsp;

--------------------------------------------------------------------------------
## Spans

Spans are just pairs of pointers that delimit a range of atoms: 'span.begin' is inclusive, 'span.end' is exclusive. So a span contains atoms in the range [begin, end).

Matchers can return a span unchanged to indicate a match that did not consume any input, a 'fail' span to indicate a match failure, or a span with the 'begin' pointer advanced by some number of atoms.

Fail spans have 'begin' set to nullptr and 'end' set to the location in the input where the match failed.

```cpp
const char* some_text = "Hello World";
matcheroni::Span<char> some_span(some_text, some_text + strlen(some_text));

printf("valid %d\n", some_span.is_valid());  // prints '1'
printf("empty %d\n", some_span.is_empty());  // prints '0'
printf("len   %d\n", some_span.len());       // prints '11'
printf("text  %s\n", some_span.begin);       // prints 'Hello World'

matcheroni::Span<char> next_span = some_span.advance(3);
printf("valid %d\n", next_span.is_valid());  // prints '1'
printf("empty %d\n", next_span.is_empty());  // prints '0'
printf("len   %d\n", next_span.len());       // prints '8'
printf("text  %s\n", next_span.begin);       // prints 'lo World'

matcheroni::Span<char> fail_span = next_span.fail();
printf("valid %d\n", fail_span.is_valid());  // prints '0'
printf("empty %d\n", fail_span.is_empty());  // prints '0'
printf("begin %s\n", fail_span.begin);       // prints '(null)'
printf("end   %s\n", fail_span.end);         // prints 'lo World'

matcheroni::Span<char> end_span = next_span.advance(8);
printf("valid %d\n", end_span.is_valid());   // prints '1'
printf("empty %d\n", end_span.is_empty());   // prints '1'
printf("len   %d\n", end_span.len());        // prints '0'
printf("text  %s\n", end_span.begin);        // prints ''
```

&nbsp;

#### Why didn't you just use std::span<>?
It pulls in a bunch of other STL header files and stuff, and I want Matcheroni
to have as few dependencies as possible.

&nbsp;

--------------------------------------------------------------------------------
## Recursion + Ref<>

C++ templates cannot accept themselves as arguments. To work around this,
Matcheroni includes a Ref<> template that can be used to wrap a free function or
a context member function and add it to a pattern.

Using Ref<> with free functions:
```cpp
TextSpan match_letter_q(TextMatchContext& ctx, TextSpan body) {
  return body.begin[0] == 'q' ? body.advance(1) : body.fail();
}
using letter_q = Ref<match_letter_q>;
using some_q = Some<letter_q>;
```

Using Ref<> with context member functions - note that 'match_letter_q' does not
have a context parameter, since it's _in_ the context:
```cpp
struct MyContext {
  TextSpan match_letter_q(TextSpan body) {
    return body.begin[0] == 'q' ? body.advance(1) : body.fail();
  }
};
using letter_q = Ref<&MyContext::match_letter_q>;
using some_q = Some<letter_q>;
```

And here's how to use Ref<> to implement a recursive matcher:

```cpp
TextSpan match_parens(TextMatchContext& ctx, TextSpan body);
using parens = Ref<match_parens>;

TextSpan match_parens(TextMatchContext& ctx, TextSpan body) {
  using pattern = Seq< Atom<'('>, Opt<parens>, Atom<')'> >;
  return pattern::match(ctx, body);
}
```

If your matcher function is inside a struct, you can skip the predeclaration:
```cpp
struct MyMatcher {
  static TextSpan match_parens(TextMatchContext& ctx, TextSpan body) {
    using pattern = Seq< Atom<'('>, Opt<parens>, Atom<')'> >;
    return pattern::match(ctx, body);
  }
  using parens = Ref<match_parens>;
};
```

&nbsp;

--------------------------------------------------------------------------------
## Store/MatchBackref

StoreBackref<x> / MatchBackref<x> works like backreferences in regex, with a
caveat - the backref is stored as a static variable _in_ the matcher's struct,
so be careful with nesting these as you could clobber a backref on accident.

&nbsp;

--------------------------------------------------------------------------------
## PatternWrapper<a>

&nbsp;

--------------------------------------------------------------------------------
## NodeBase

NodeBase<> is a templated mixin-style parse node base class that stores the
child/sibling pointers for each parse node, along with a tag string and the
span from the matcher.

You inherit from it like so:

```
struct TextParseNode : public NodeBase<TextParseNode, char> {...
```

#### NodeBase methods

```cpp
// Initializes the node _after_ its sibling/child pointers are connected.
// Override this in your node class to do additional stuff when a parse node is
// created.
void init(const char* match_name, SpanType span, uint64_t flags);

// Returns the first child whose "match_name" field is "name"
NodeType* child(const char* name);

// Returns the number of nodes under this node, inclusive (no children = 1)
size_t node_count();
```


#### NodeBase members

```cpp
// The name assigned to this node from the Capture<"name", ...> wrapper
const char* match_name;

// The range of atoms this node is associated with
SpanType span;

// General-purpose flags. CaptureBegin<> and CaptureEnd<> use this to denote a
// temporary node as a 'bookmark'
uint64_t flags;

NodeType* node_prev;  // Previous sibling node, or nullptr
NodeType* node_next;  // Next sibling node, or nullptr
NodeType* child_head; // First child node, or nullptr
NodeType* child_tail; // Last child node, or nullptr
```

&nbsp;

--------------------------------------------------------------------------------
## NodeContext

NodeContext<> handles all the pointer bookkeeping needed to create parse trees and uses a specialized allocator to minimize the overhead of creating and destroying parse nodes as we work through all our possible matches.

During parsing, NodeContext's "top_head" and "top_tail" pointers contain a doubly-linked list of intermediate parse trees that will be coalesced into larger trees as parsing continues.

After parsing, "top_head" and "top_tail" contain a list of root nodes for all valid parse trees found in the input span.

#### NodeContext template arguments

```cpp
template <
  // The base type common to all nodes in this parse tree.
  // Individual parse nodes can be subclasses of this type.
  typename _NodeType,

  // Whether to call constructors when creating a new node. Defaults to true.
  // If NodeType has virtual methods, this _must_ be true.
  bool _call_constructors = true,

  // Whether to call destructors when deleting a node. Defaults to true.
  // If NodeType allocates resources, this _must_ be true.
  bool _call_destructors = true
>
struct NodeContext {

  NodeContext();
  ~NodeContext();
  void reset();
  size_t node_count();
  void append(NodeType* new_node);
  void detach(NodeType* n);
  void splice(NodeType* new_node, NodeType* child_head, NodeType* child_tail);
  NodeType* checkpoint();
  void rewind(NodeType* old_tail);
  void merge_node(NodeType* new_node, NodeType* old_tail);
  NodeType* enclose_bookmark(NodeType* old_tail, NodeType::SpanType bounds);
  void recycle(NodeType* node);
```

#### NodeContext members

| foo | bar |
|-----|-----|
| LifoAlloc alloc; | A specialized, LIFO-order allocator for parse nodes. |
| NodeType* top_head; | The first node in the context's temporary node list. |
| NodeType* top_tail; | The last node in the context's temporary node list. |
| int trace_depth; | Bookkeeping for TraceText<> so the indentation appears correct |

&nbsp;

### Checkpoint / Rewind

Parseroni may get arbitrarily far down a matching path before discovering that
its current pattern does not match. At that point, it has to undo whatever state
changes were made in the process of getting there so that it can try another
path.

Because we don't want Parseroni to depend on internal implementation details of
the Context, we implement the 'undo' operation in two parts - 'checkpoint'
creates an opaque token (typed as a void*) representing the current state of the
context, and 'rewind' restores the context to a previous state. The NodeContext
implements these methods just by manipulating the 'tail' pointer of the
temporary node list:

```cpp
  NodeType* checkpoint() {
    return top_tail;
  }

  void rewind(NodeType* old_tail) {
    while(top_tail != old_tail) {
      auto dead = top_tail;
      top_tail = top_tail->node_prev;
      recycle(dead);
    }
  }
```

&nbsp;

### LifoAlloc & Node Recycling

If Parseroni used the default C++ allocator, it would spend more time allocating
and freeing parse nodes than it would doing the actual parsing.

At the same time, we don't want a simple 'bump' or 'arena' allocator - they
don't allow for deallocation, so could end up wasting huge amounts of memory on
node allocations from failed matches.

Instead we use a 'LIFO' (last-in-first-out) allocator, which is somewhere in between a stack and an
allocator. Allocations can be any size, but deallocations must be in the reverse
order of the allocations.

This makes deallocation of trees of nodes slightly tricky, but since we know
that 'parent' nodes are _always_ allocated _after_ their child nodes, we can
still deallocate everything safely. Parseroni wraps this up in a 'recycle'
method that does the right thing.

&nbsp;

--------------------------------------------------------------------------------
## Matcher Functions

&nbsp;

--------------------------------------------------------------------------------
# Additional built-in matchers for convenience
While writing the C lexer and parser demos, I found myself needing some additional pieces:

# Examples
Matcheroni patterns are roughly equivalent to regular expressions. A regular expression using the std::regex C++ library
cpp
std::regex my_pattern("[abc]+def");

would be expressed in Matcheroni as
cpp
using my_pattern = Seq<Some<Atom<'a','b','c'>>, Lit<"def">>;

In the above line of code, we are defining the matcher "my_pattern" by nesting the Seq<>, Some<>, Atom<>, and Lit<> matcher templates. The resuling type (not instance) defines a static "match()" function that behaves similarly to the regex.

Unlike std::regex, we don't need to link in any additional libraries or instantiate anything to use it:
cpp
const std::string text = "aaabbaaccdefxyz";

// The first argument to match() is a reference to a context object.
// The second two arguments are the range of text to match against.
// The match function returns the _end_ of the match, or nullptr if there was no match.
TextMatchContext ctx;
TextSpan tail = my_pattern::match(ctx, to_span(text));

// Since we matched "aabbaaccdef", this prints "xyz".
printf("%s\n", result);


Matchers are also modular - you could write the above as
cpp
using abc = Atom<'a','b','c'>;
using def = Lit<"def">;
using my_pattern = Seq<Some<abc>, def>;

and it would perform identically to the one-line version.

Unlike regexes, matchers can be recursive. Note that you can't nest a pattern inside itself directly, as "using pattern" doesn't count as a declaration. Forward-declaring a matcher function and using that in a pattern works though (and you can refer to the "using" declaration before it's declared):
cpp

static TextSpan match_parens(TextMatchContext& ctx, TextSpan body) {
  return Seq<Atom<'('>, Any<parens, NotAtom<')'>>, Atom<')'>>::match(ctx, body);
}
using parens = Ref<match_parens>;

// Now we can use the pattern
std::string text = "(((foo)bar)baz)tail";
TextMatchContext ctx;
TextSpan tail = parens::match(ctx, to_span(text));
printf("%s", tail); // prints "tail"

&nbsp;
