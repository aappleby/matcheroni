------
### TL;DR - Copy-paste [Matcheroni.hpp](matcheroni/Matcheroni.hpp) into your C++20 project and you can build simple [parsers](matcheroni/regex_parser.cpp) without needing a parser generator or a regex library.
------

# Matcheroni

Matcheroni is a minimal, zero-dependency (not even stdlib), single-file header library for building pattern-matchers, [lexers](matcheroni/examples/c_parser/CLexer.cpp), and [parsers](matcheroni/examples/c_parser/CContext.h) out of trees of C++20 templates.

Matcheroni is a generalization of [Parsing Expression Grammars](https://en.wikipedia.org/wiki/Parsing_expression_grammar) and can be used in place of regular expressions in many cases.

Matcheroni generates tiny code - 100s of bytes for moderately-sized patterns versus 100k+ if you have to include the std::regex library.

Matcheroni generates fast code - often 10x faster than std::regex.

Matcheroni matchers are more readable and more modular than regexes - you can build [large matchers](matcheroni/examples/c_parser/CLexer.cpp) out of small simple matchers without affecting performance.

Matcheroni allows you to freely intermingle C++ code with your matcher templates so that you can build parse trees, log stats, or do whatever else you need to do while processing your data.

Matcheroni doesn't have to match text - you can customize it to match patterns in arrays of any type as long as you define appropriate comparison functions for your types.

# Examples
Matchers are roughly equivalent to regular expressions. A regular expression using the std::regex C++ library
```cpp
std::regex my_pattern("[abc]+def");
```
would be expressed in Matcheroni as
```cpp
using my_pattern = Seq<Some<Atom<'a','b','c'>>, Lit<"def">>;
```
In the above line of code, we are defining the matcher "my_pattern" by nesting the Seq<>, Some<>, Atom<>, and Lit<> matcher templates. The resuling type (not instance) defines a static "match()" function that behaves similarly to the regex.

Unlike std::regex, we don't need to link in any additional libraries or instantiate anything to use it:
```cpp
const std::string text = "aaabbaaccdefxyz";

// The first argument to match() is a user-defined context pointer.
// The second two arguments are the range of text to match against.
// The match function returns the _end_ of the match, or nullptr if there was no match.
const char* result = my_pattern::match(nullptr, text.data(), text.data() + text.size());

// Since we matched "aabbaaccdef", this prints "xyz".
printf("%s\n", result);
```

Matchers are also modular - you could write the above as
```cpp
using abc = Atom<'a','b','c'>;
using def = Lit<"def">;
using my_pattern = Seq<Some<abc>, def>;
```
and it would perform identically to the one-line version.

Unlike regexes, matchers can be recursive. Note that you can't nest a pattern inside itself directly, as "using pattern" doesn't count as a declaration. Forward-declaring a matcher function and using that in a pattern works though:
```cpp
// Forward-declare our matching function so we can use it recursively.
const char* match_parens_recurse(void* ctx, const char* a, const char* b);

// Define our recursive matching pattern.
using match_parens =
Seq<
  Atom<'('>,
  Any<Ref<match_parens_recurse>, NotAtom<')'>>,
  Atom<')'>
>;

// Implement the forward-declared function by recursing into the pattern.
const char* match_parens_recurse(void* ctx, const char* a, const char* b) {
  return match_parens::match(ctx, a, b);
}

// Now we can use the pattern
std::string text = "(((foo)bar)baz)tail";
auto end = match_parens::match(nullptr, text.data(), text.data() + text.size());
printf("%s", end); // prints "tail"
```
# Building the Matcheroni examples
Install [Ninja](https://ninja-build.org/) if you haven't already, then run ninja in the repo root.

See build.ninja for configuration options.

# It's all a bunch of templates? Really?
If you're familiar with C++ templates, you are probably concerned that this library will cause your binary size to explode and your compiler to grind to a halt.

I can assure you that that's not the case - binary sizes and compile times for even pretty large matchers are fine, though the resulting debug symbols are so enormous as to be useless.

# Fundamentals

Matcheroni is based on two fundamental primitives -

 - A **"matching function"** is a function of the form ```atom* match(void* ctx, atom* a, atom* b)```, where ```atom``` can be any data type you can store in an array, ```ctx``` is an opaque pointer to whatever bookkeeping data structure your app requires, and ```a``` and ```b``` are the endpoints of the range of atoms in the array to match against.

 - A **"matcher"** is any class or struct that defines a static matching function named ```match()```.

Matching functions should return a pointer in the range ```[a, b]``` to indicate success or failure - returning ```a``` means the match succeded but did not consume any input, returning ```b``` means the match consumed all the input, returning ```nullptr``` means the match failed, and any other pointer in the range indicates that the match succeeded and consumed some amount of the input.

Matcheroni includes [built-in matchers for most regex-like tasks](matcheroni/Matcheroni.h#L54), but writing your own is straightforward. Matchers can be templated and can do basically whatever they like inside ```match()```. For example, if we wanted to print a message whenever some pattern matches, we could do this:

```cpp
template<typename P>
struct PrintMessage {
  static const char* match(void* ctx, const char* a, const char* b) {
    const char* result = P::match(ctx, a, b);
    if (result) {
      printf("Match succeeded!\n");
    }
    else {
      printf("Match failed!\n");
    }
    return result;
  }
};
```

and we could use it like this:
```cpp
using pattern = PrintMessage<Atom<'a'>>;
const std::string text = "This does not start with 'a'";

// prints "Match failed!"
pattern::match(nullptr, text.data(), text.data() + text.size());
```

# Built-in matchers
Since Matcheroni is based on Parsing Expression Grammars, it includes all the basic rules you'd expect:

- ```Atom<x, y, ...>``` matches any single "atom" of your input that is equal to one of the template parameters. Atoms can be characters, objects, or whatever as long as you implement ```atom_cmp(...)``` for them. Atoms "consume" input and advance the read cursor when they match.
- ```Seq<x, y, ...>``` matches sequences of other matchers.
- ```Oneof<x, y, ...>``` returns the result of the first successful sub-matcher. Like parsing expression grammars, there is no backtracking - if ```x``` matches, we will never back up and try ```y```.
- ```Any<x>``` is equivalent to ```x*``` in regex - it matches zero or more instances of ```x```.
- ```Some<x>``` is equivalent to ```x+``` in regex - it matches one or more instances of ```x```.
- ```Opt<x>``` is equivalent to ```x?``` in regex - it matches exactly 0 or 1 instances of ```x```.
- ```And<x>``` matches ```x``` but does _not_ advance the read cursor.
- ```Not<x>``` matches if ```x``` does _not_ match and does _not_ advance the read cursor.

# Additional built-in matchers for convenience
While writing the C lexer and parser demos, I found myself needing some additional pieces:

- ```Rep<N, x>``` matches ```x``` N times.
- ```NotAtom<x, ...>``` is equivalent to ```[^abc]``` in regex, and is faster than ```Seq<Not<Atom<x, ...>>, AnyAtom>```
- ```Range<x, y>``` is equivalent to ```[x-y]``` in regex.
- ```Until<x>``` matches anything until X matches, but does not consume X. Equivalent to ```Any<Seq<Not<x>,AnyAtom>>```
- ```Ref<f>``` allows you to plug arbitrary code into a tree of matchers as long as ```f``` is a valid matching function. Ref can store pointers to free functions or member functions, but if you use member functions be sure to pass a pointer to the source object into the matcher via the ```ctx``` param.
- ```StoreBackref<x> / MatchBackref<x>``` works like backreferences in regex, with a caveat - the backref is stored as a static variable _in_ the matcher's struct, so be careful with nesting these as you could clobber a backref on accident.
- ```EOL``` matches newlines and end-of-file, but does not advance past it.
- ```Lit<x>``` matches a C string literal (only valid for ```char``` atoms)
- ```Keyword<x>``` matches a C string literal as if it was a single atom - this is only useful if your atom type can represent whole strings.
- ```Charset<x>``` matches any ```char``` atom in the string literal ```x```, which can be much more concise than ```Atom<'a', 'b', 'c', 'd', ...>```
- ```Map<x, ...>``` differs from the other matchers in that expects ```x``` to define both ```match()``` and ```match_key()```. ```Map<>``` is like ```Oneof<>``` except that it checks ```match_key()``` first and then returns the result of ```match()``` if the key pattern matched. It does _not_ check other alternatives once the key pattern matches. This should allow for more performant matchers, but I haven't used it much yet.

# Performance

After compilation, the trees of templates turn into trees of tiny simple function calls. GCC and Clang do an exceptionally good job of optimizing these down into functions that are nearly as small and fast as if you'd written them by hand. The generated assembly looks good, and the code size can actually be smaller than hand-written as GCC can dedupe redundant template instantiations in a lot of cases.

Matcheroni includes a small [benchmark](matcheroni/benchmark.cpp) that compares build time, binary size, and performance against some other popular header-only regex libraries.

[Results of the performance comparison are here](https://docs.google.com/spreadsheets/d/17AjRa8XYFfhlluFPoLMWJUpjH6aI_gf-psUpRgDJIUA/edit?usp=sharing)

Overall results:

 - [Matcheroni](https://github.com/aappleby/Matcheroni) adds very little to build time or binary size. Its performance compares favorably with CTRE and Boost, and is vastly faster than std::regex.
 - [SRELL](https://www.akenotsuki.com/misc/srell/en/) is the performance champion but adds a lot to build time and binary size by default.
 - [SRELL](https://www.akenotsuki.com/misc/srell/en/#smaller) in 'minimized' mode is smaller than Boost or std::regex but is still slow to build.
 - [Boost](https://www.boost.org/doc/libs/1_82_0/libs/regex/doc/html/index.html) is fast, has a large impact on build time, and a moderate impact on binary size.
 - [CTRE](https://github.com/hanickadot/compile-time-regular-expressions) is fast, has a large impact on build time, but doesn't add much to the binary size.
 - [std::regex](https://en.cppreference.com/w/cpp/regex) is terrible by all metrics.

So, if you need to do some customized pattern-matching on something like an embedded platform and you want to keep your compile-test cycle fast, give Matcheroni a try.

# A Small Demo - Parsing Regular Expressions

There is a full working example of using Matcheroni to parse a subset of regular expression syntax, build a syntax tree, print the tree, and (optionally) trace the matching process in [regex_parser.cpp](matcheroni/regex_parser.cpp).

```
~/Matcheroni$ bin/regex_parser "[a-zA-Z]*(foobarbaz|glom.*)?"
argv[0] = bin/regex_parser
argv[1] = [a-zA-Z]* (foobarbaz|glom.*)?

Parse tree:
[a-zA-Z]*              any
[a-zA-Z]               |--pos_set
a-z                    |  |--range
a                      |  |  |--begin
z                      |  |  |--end
A-Z                    |  |--range
A                      |  |  |--begin
Z                      |  |  |--end
(foobarbaz|glom.*)?    opt
(foobarbaz|glom.*)     |--group
foobarbaz|glom.*       |  |--oneof
foobarbaz              |  |  |--option
foobarbaz              |  |  |  |--text
glom.*                 |  |  |--option
glom                   |  |  |  |--text
.*                     |  |  |  |--any
.                      |  |  |  |  |--dot
```

This should suffice to cover basic and intermediate usage of Matcheroni, including recursive matching and implementing custom matchers that maintain global state.

# A Larger Demo - Lexing and Parsing C
This repo contains a work-in-progress example C lexer and parser built using Matcheroni.

The lexer should be conformant to the C99 spec, the parser is less conformant but is still able to parse nearly everything in GCC's torture-test suite.

The output of the parser is a simple tree of parse nodes with all parent/child/sibling links as pointers:

Here's our parser for C's ```for``` loops:
```cpp
struct NodeStatementFor : public ParseNode, public NodeMaker<NodeStatementFor> {
  using pattern =
  Seq<
    Keyword<"for">,
    Atom<'('>,
    Oneof<
      Seq<comma_separated<NodeExpression>,  Atom<';'>>,
      Seq<comma_separated<NodeDeclaration>, Atom<';'>>,
      Atom<';'>
    >,
    Opt<comma_separated<NodeExpression>>,
    Atom<';'>,
    Opt<comma_separated<NodeExpression>>,
    Atom<')'>,
    Oneof<NodeStatementCompound, NodeStatement>
  >;
};
```
Note that there's no code or data in the class. That's intentional - the NodeMaker<> helper only requires that a parse node type declares a match pattern and it will take care of the details of matching source code, creating parse nodes, and linking them together into a tree.

# Caveats

Matcheroni requires C++20, which is a non-starter for some projects. There's not a lot I can do about that, as I'm heavily leveraging some newish template stuff that doesn't have any backwards-compatible equivalents.

Like parsing expression grammars, matchers are greedy - ```Seq<Some<Atom<'a'>>, Atom<'a'>>``` will _always_ fail as ```Some<Atom<'a'>>``` leaves no 'a's behind for the second ```Atom<'a'>``` to match.

Matcheroni does not implement any form of [packrat parsing](https://pdos.csail.mit.edu/~baford/packrat/icfp02/), though it could be added on top. Trying to do [operator-precedence parsing](https://en.wikipedia.org/wiki/Operator-precedence_parser) using the precedence-climbing method will be unbearably slow due to the huge number of recursive calls that don't end up matching anything.

Recursive matchers create recursive code that can explode your call stack.

Left-recursive matchers can get stuck in an infinite loop - this is true with most versions of Parsing Expression Grammars, it's a fundamental limitation of the algorithm.

Parsers generated with a real parser generator will probably be faster.

# A Particularly Large Matcheroni Pattern

Here's the code I use to match C99 integers, plus a few additions from the C++ spec and the GCC extensions.

Note that it consists of 20 ```using``` declarations and the only actual "code" is ```return integer_constant::match(ctx, a, b);```

If you follow along in Appendix A of the [C99 spec](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf), you'll see it lines up quite closely.

```cpp
const char* match_int(void* ctx, const char* a, const char* b) {
  // clang-format off
  using digit                = Range<'0', '9'>;
  using nonzero_digit        = Range<'1', '9'>;

  using decimal_constant     = Seq<nonzero_digit, Any<ticked<digit>>>;

  using hexadecimal_prefix         = Oneof<Lit<"0x">, Lit<"0X">>;
  using hexadecimal_digit          = Oneof<Range<'0', '9'>, Range<'a', 'f'>, Range<'A', 'F'>>;
  using hexadecimal_digit_sequence = Seq<hexadecimal_digit, Any<ticked<hexadecimal_digit>>>;
  using hexadecimal_constant       = Seq<hexadecimal_prefix, hexadecimal_digit_sequence>;

  using binary_prefix         = Oneof<Lit<"0b">, Lit<"0B">>;
  using binary_digit          = Atom<'0','1'>;
  using binary_digit_sequence = Seq<binary_digit, Any<ticked<binary_digit>>>;
  using binary_constant       = Seq<binary_prefix, binary_digit_sequence>;

  using octal_digit        = Range<'0', '7'>;
  using octal_constant     = Seq<Atom<'0'>, Any<ticked<octal_digit>>>;

  using unsigned_suffix        = Atom<'u', 'U'>;
  using long_suffix            = Atom<'l', 'L'>;
  using long_long_suffix       = Oneof<Lit<"ll">, Lit<"LL">>;
  using bit_precise_int_suffix = Oneof<Lit<"wb">, Lit<"WB">>;

  // This is a little odd because we have to match in longest-suffix-first order
  // to ensure we capture the entire suffix
  using integer_suffix = Oneof<
    Seq<unsigned_suffix,  long_long_suffix>,
    Seq<unsigned_suffix,  long_suffix>,
    Seq<unsigned_suffix,  bit_precise_int_suffix>,
    Seq<unsigned_suffix>,

    Seq<long_long_suffix,       Opt<unsigned_suffix>>,
    Seq<long_suffix,            Opt<unsigned_suffix>>,
    Seq<bit_precise_int_suffix, Opt<unsigned_suffix>>
  >;

  // GCC allows i or j in addition to the normal suffixes for complex-ified types :/...
  using complex_suffix = Atom<'i', 'j'>;

  // Octal has to be _after_ bin/hex so we don't prematurely match the prefix
  using integer_constant =
  Seq<
    Oneof<
      decimal_constant,
      hexadecimal_constant,
      binary_constant,
      octal_constant
    >,
    Seq<
      Opt<complex_suffix>,
      Opt<integer_suffix>,
      Opt<complex_suffix>
    >
  >;

  return integer_constant::match(ctx, a, b);
  // clang-format on
}
```
