# Matcheroni
Matcheroni is a minimal, zero-dependency (not even stdlib), header-only library for building pattern-matchers, [lexers](matcheroni/c_lexer.cpp), and [parsers](matcheroni/c_parser.h) out of trees of C++20 templates.

Matcheroni is a generalization of [Parsing Expression Grammars](https://en.wikipedia.org/wiki/Parsing_expression_grammar) and can be used in place of regular expressions in most cases.

Matcheroni generates tiny code - 100s of bytes for moderately-sized patterns versus 100k+ if you have to include the std::regex library.

Matcheroni generates fast code - 10x or more versus std::regex.

Matcheroni matchers are more readable and more modular than regexes - you can build [large matchers](matcheroni/c_lexer.cpp#L180) out of small simple matchers without affecting performance.

Matcheroni allows you to freely intermingle C++ code with your matcher templates so that you can build parse trees, log stats, or do whatever else you need to do while processing your data.

# Examples
Matchers are roughly equivalent to regular expressions. A regular expression using the std::regex C++ library
```
std::regex my_pattern("[abc]+def");
```
would be expressed in Matcheroni as
```
using my_pattern = Seq<Some<Atom<'a','b','c'>>, Lit<"def">>;
```
In the above line of code, we are defining the matcher "my_pattern" by nesting the Seq<>, Some<>, Atom<>, and Lit<> matcher templates. The resuling type (not instance) defines a static "match()" function that behaves similarly to the regex.

Unlike std::regex, we don't need to link in any additional libraries or instantiate anything to use it:
```
const std::string text = "aaabbaaccdefxyz";

// The match function returns the _end_ of the match, or nullptr if there was no match.
const char* result = my_pattern::match(text.data(), text.data() + text.size());

// Since we matched "aabbaaccdef", this prints "xyz".
printf("%s\n", result);
```

Matchers are also modular - you could write the above as
```
using abc = Atom<'a','b','c'>;
using def = Lit<"def">;
using my_pattern = Seq<Some<abc>, def>;
```
and it would perform identically to the one-line version.

Unlike regexes, matchers can be recursive:
```
// Recursively match correctly-paired nested parenthesis
const char* matcheroni_match_parens(const char* a, const char* b) {
  using pattern =
  Seq<
    Atom<'('>,
    Any<Oneof<Ref<matcheroni_match_parens>, NotAtom<')'>>>,
    Atom<')'>
  >;
  return pattern::match(a, b);
}
```
Note that you can't nest "pattern" inside itself directly, as "using pattern" doesn't count as a declaration. Wrapping it in a function or class works though.

# Fundamentals

Matcheroni is based on two fundamental primitives -

 - A **"matching function"** is a function of the form ```atom* match(atom* a, atom* b)```, where ```atom``` can be any data type you can store in an array and ```a``` and ```b``` are the endpoints of the range of atoms in the array to match against.

Matching functions should return a pointer in the range ```[a, b]``` to indicate success or failure - returning ```a``` means the match succeded but did not consume any input, returning ```b``` means the match consumed all the input, returning ```nullptr``` means the match failed, and any other pointer in the range indicates that the match succeeded and consumed some amount of the input.

 - A **"matcher"** is any class or struct that defines a static matching function named ```match()```.

Matcheroni includes [built-in matchers for most regex-like tasks](matcheroni/Matcheroni.h#L54), but writing your own is straightforward. Matchers can be templated and can do basically whatever they like inside ```match()```. For example, if we wanted to print a message whenever some pattern matches, we could do this:

```
template<typename P>
struct PrintMessage {
  static const char* match(const char* a, const char* b) {
    const char* result = P::match(a, b);
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
```
using pattern = PrintMessage<Atom<'a'>>;
const std::string text = "This does not start with 'a'";

// prints "Match failed!"
pattern::match(text.data(), text.data() + text.size());
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

- ```SomeOf<x, ...>``` is just ```Some<Of<x, ...>>```
- ```Rep<N, x>``` matches ```x``` N times.
- ```NotAtom<x, ...>``` is equivalent to ```[^abc]``` in regex, and is faster than ```Seq<Not<Atom<x, ...>>, AnyAtom>```
- ```Range<x, y>``` is equivalent to ```[x-y]``` in regex.
- ```Until<x>``` matches anything until X matches, but does not consume X. Equivalent to ```Any<Seq<Not<x>,AnyAtom>>```
- ```Ref<f>``` allows you to plug arbitrary code into a tree of matchers as long as ```f``` is a valid matching function.
- ```StoreBackref<x> / MatchBackref<x>``` works like backreferences in regex, with a caveat - the backref is stored as a static variable _in_ the matcher's struct, so be careful with nesting these as you could clobber a backref on accident.
- ```EOL``` matches newlines and end-of-file, but does not advance past it.
- ```Lit<x>``` matches a C string literal (only valid for ```char``` atoms)
- ```Keyword<x>``` matches a C string literal as if it was a single atom - this is only useful if your atom type can represent whole strings.
- ```Charset<x>``` matches any ```char``` atom in the string literal ```x```, which can be much more concise than ```Atom<'a', 'b', 'c', 'd', ...>```
- ```Map<x, ...>``` differs from the other matchers in that expects ```x``` to define both ```match()``` and ```match_key()```. ```Map<>``` is like ```Oneof<>``` except that it checks ```match_key()``` first and then returns the result of ```match()``` if the key pattern matched. It does _not_ check other alternatives once the key pattern matches. This should allow for more performant matchers, but I haven't used it much yet.

# Performance
After compilation, the trees of templates turn into trees of tiny simple function calls. GCC and Clang do an exceptionally good job of optimizing these down into functions that are nearly as small and fast as if you'd written them by hand. The generated assembly looks good, and the code size can actually be smaller than hand-written as GCC can dedupe redundant template instantiations in a lot of cases.

I've written a quick benchmark that generates a 1 million character string consisting of letters and parenthesis, with a maximum parenthesis nesting depth of 5.

For the regex pattern ```\([^()]+\)``` and the equivalent Matcheroni pattern ```Seq<Atom<'('>, Some<NotAtom<'(', ')'>>, Atom<')'>>``` (paired parenthesis containing only non-parenthesis):
```
Matcheroni is 153.891915 times faster than std::regex_search
Matcheroni is 10.277835 times faster than std::regex_iterator
Matcheroni is 1.012847 times faster than hand-written
```

I also tested the recursive parenthesis matcher above against a recursive hand-written implementation and a non-recursive hand-written implementation.
```
With -O3:
matcheroni_match_parens: 123 bytes
recursive_matching_parens: 81 bytes
nonrecursive_matching_parens: 75 bytes
matcheroni_match_parens is 0.692946 times faster than recursive_matching_parens
matcheroni_match_parens is 0.578635 times faster than nonrecursive_matching_parens
```
The Matcheroni version is a bit larger and slower due to extra null and range checking, but I didn't have to worry about termination conditions, null pointers, or off-by-ones.

It's hard to measure exactly how much the std::regex library adds to the binary, but we can compare against a build with the std::regex benchmarks commented out:
```
With std::regex, -O3
-rwxr-xr-x 1 aappleby aappleby 174232 Jun 20 14:11 benchmark

With std::regex, -Os
-rwxr-xr-x 1 aappleby aappleby 140536 Jun 20 14:19 benchmark

Without std::regex, -O3
-rwxr-xr-x 1 aappleby aappleby  17664 Jun 20 14:15 benchmark

Without std::regex, -Os
-rwxr-xr-x 1 aappleby aappleby  17496 Jun 20 14:18 benchmark
```
So std::regex adds about 130k-150k of code for this example.

# Templates? Really?
If you're familiar with C++ templates, you are probably concerned that this library will cause your compiler to grind to a halt. I can assure you that that's not the case - compile times for even pretty large matchers are fine, though the resulting debug symbols are so enormous as to be useless.

# Demo - Lexing and Parsing C
This repo contains an example C lexer and parser built using Matcheroni. The lexer should be conformant to the C99 spec, the parser is less conformant but is still able to parse nearly everything in GCC's torture-test suite. The output of the parser is a simple tree of parse nodes with all parent/child/sibling links as pointers.

Here's our parser for C's ```for``` loops:
```
struct NodeStatementFor : public NodeMaker<NodeStatementFor> {
  using pattern = Seq<
    Keyword<"for">,
    Atom<'('>,
    Opt<comma_separated<Oneof<
      NodeExpression,
      NodeDeclaration
    >>>,
    Atom<';'>,
    Opt<comma_separated<NodeExpression>>,
    Atom<';'>,
    Opt<comma_separated<NodeExpression>>,
    Atom<')'>,
    NodeStatement
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

# A non-trivial example

Here's the code I use to match C99 integers, plus a few additions from the C++ spec and the GCC extensions.

If you follow along in Appendix A of the [C99 spec](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf), you'll see it lines up quite closely.

```
const char* match_int(const char* a, const char* b) {
  using digit         = Range<'0', '9'>;
  using nonzero_digit = Range<'1', '9'>;

  // The 'ticked' template allows for an optional single-quote to be inserted
  // before each digit.
  using decimal_constant = Seq<nonzero_digit, Any<ticked<digit>>>;

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

  return integer_constant::match(a, b);
}
```
