# Matcheroni
Matcheroni is a minimal, zero-dependency (not even stdlib), header-only library for building text matchers, lexers, and parsers out of trees of C++20 templates. It's a generalization of Parsing Expression Grammars, and allows you to freely mix hand-written matching code and templated match patterns.

# Example
Matcheroni patterns are roughly equivalent to regular expressions - something like
```
regex my_pattern("[abc]+def");
```
would be expressed in Matcheroni as
```
using my_pattern = Seq<Some<Atom<'a','b','c'>>, Lit<"def">>;
```
Unlike std::regex though, the Matcheroni pattern is both the definition of the pattern **and** the implementation of the matcher for that pattern.

Matcheroni patterns thus require no additional libraries or code to be usable - you can immediately call
```
const std::string text = "aaabbaaccdefxyz";
const char* result = my_pattern::match(text.data(), text.data() + text.size());
printf("%s\n", result); // prints "xyz"
```

# Fundamentals

Matcheroni is based on two fundamental primitives -

- A "matching function" is a function of the form ```const atom* match(const atom* a, const atom* b)```, where ```atom``` can be any plain-old-data type and ```a``` and ```b``` are the endpoints of the range of atoms to match against. Matching functions should return a pointer in the range ```[a, b]``` to indicate success or failure - returning ```a``` means the match succeded but did not consume any input, returning ```b``` means the match consumed all the input, returning ```nullptr``` means the match failed, and any other pointer in the range indicates that the match succeeded and consumed some amount of the input.

- A "matcher" is any class or struct that contains a matching function named ```match()``` in its body. Matchers can be templated and can do basically whatever they like inside ```match()```. For example, if we wanted to print a message whenever some pattern matches, we could do this:

```
template<typename P>
struct PrintMessage {
  const char* match(const char* a, const char* b) {
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
pattern::match(text.data(), text.data() + text.size()); // prints "Match failed!"
```



# Templates? Really?
If you're familiar with C++ templates, you are probably concerned that this library will cause your compiler to grind to a halt. I can assure you that that's not the case - compile times for even pretty large matchers are fine, though the resulting debug symbols are so enormous as to be useless.

# Built-in patterns

Since Matcheroni is based on Parsing Expression Grammars, it includes all the basic rules you'd expect:

- ```Atom<x, y, ...>``` matches any single "atom" of your input that is equal to one of the template parameters. Atoms can be characters, objects, or whatever as long as you implement ```atom_eq(...)``` for them. Atoms "consume" input and advance the read cursor when they match.
- ```Seq<x, y, ...>``` matches sequences of other matchers.
- ```Oneof<x, y, ...>``` returns the result of the first pattern that matches the input. Like parsing expression grammars, there is no backtracking - if ```x``` matches, we will never back up and try ```y```.
- ```Any<x>``` is equivalent to ```x*``` in regex - it matches zero or more instances of ```x```.
- ```Some<x>``` is equivalent to ```x+``` in regex - it matches one or more instances of ```x```.
- ```Opt<x>``` is equivalent to ```x?``` in regex - it matches exactly 0 or 1 instances of ```x```.
- ```And<x>``` matches ```x``` but does _not_ advance the read cursor.
- ```Not<x>``` matches if ```x``` does _not_ match and does _not_ advance the read cursor.

# Additional built-in patterns for convenience

While writing the C lexer and parser demos, I found myself needing some additional matchers:

- ```SomeOf<x, ...>``` is just ```Some<Of<x, ...>>```
- ```Rep<N, x>``` matches ```x``` N times.
- ```NotAtom<x, ...>``` is equivalent to ```[^abc]``` in regex, and is faster than ```Seq<Not<Atom<x, ...>>, AnyAtom>```
- ```Range<x, y>``` is equivalent to ```[x-y]``` in regex.
- ```Until<x>``` matches anything until X matches, but does not consume X. Equivalent to ```Any<Seq<Not<x>,AnyAtom>>```
- ```Ref<f>``` allows you to plug hand-written matching code into a Matcheroni pattern. The function ```f``` must be of the form ```const atom* match(const atom* a, const atom* b)```
- ```StoreBackref<x> / MatchBackref<x>``` works like backreferences in regex, with a caveat - the backref is stored as a static variable _in_ the matcher's struct, so be careful with nesting these as you could clobber a backref on accident.
- ```EOL``` matches newlines and end-of-file, but does not advance past it.
- ```Lit<x>``` matches a C string literal (only valid for ```const char``` atoms)
- ```Keyword<x>``` matches a C string literal as if it was a single atom - this is only useful if your atom type can represent whole strings.
- ```Charset<x>``` matches any ```const char``` atom in the string literal ```x```, which can be much more concise than ```Atom<'a', 'b', 'c', 'd', ...>```
- ```Map<x, ...>``` differs from the other matchers in that expects ```x``` to define both ```match()``` and ```match_key()```. ```Map<>``` is like ```Oneof<>``` except that it checks ```match_key()``` first and then returns the result of ```match()``` if the key pattern matched. It does _not_ check other alternatives once the key pattern matches. This should allow for more performant matchers, but I haven't used it much yet.

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


# Performance
After compilation, the trees of templates turn into trees of tiny simple function calls. GCC/Clang's optimizer does an exceptionally good job of flattening these down into small optimized functions that are nearly as small and fast as if you'd written the matchers by hand. The generated assembly looks good, and the code size can actually be smaller than hand-written as GCC can dedupe redundant template instantiations in a lot of cases.

I've written a quick benchmark that generates a 1 million character string consisting of letters and parenthesis, with a maximum parenthesis nesting depth of 5.

For the regex pattern ```\([^()]+\)``` and the equivalent Matcheroni pattern ```Seq<Atom<'('>, Some<NotAtom<'(', ')'>>, Atom<')'>>``` (paired parenthesis containing only non-parenthesis):

```
Matcheroni is 153.891915 times faster than std::regex_search
Matcheroni is 10.277835 times faster than std::regex_iterator
Matcheroni is 1.012847 times faster than hand-written
```

I also tested matching nested paired parenthesis using a recursive Matcheroni pattern, a recursive hand-written implementation and a non-recursive hand-written implementation. The optimized Matcheroni function is 123 bytes, the non-recursive function is 75 bytes, and the recursive function is 81 bytes.

```
Matcheroni is 0.692946 times faster than hand-written recursive
Matcheroni is 0.578635 times faster than hand-written non-recursive
```

It's hard to measure exactly how much the std::regex library adds to the binary, but a build with std::regex is 174232 bytes and a build with std::regex commented out is 17664, or about 156000 bytes added.

# Caveats

Matcheroni requires C++20, which is a non-starter for some projects. There's not a lot I can do about that, as I'm heavily leveraging some newish template stuff that doesn't have any backwards-compatible equivalents.

Recursive patterns create recursive code that can explode your call stack.

Left-recursive patterns can get stuck in an infinite loop - this is true with most versions of Parsing Expression Grammars, it's a fundamental limitation of the algorithm.

Parsers generated with a real parser generator will probably be faster.

# A non-trivial example

Here's the pattern I use to match C99 integers, plus a few additions from the C++ spec and the GCC extensions:

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
  using complex_suffix = Oneof<Atom<'i'>, Atom<'j'>>;

  // Octal has to be _after_ bin/hex so we don't prematurely match the prefix
  using integer_constant = Oneof<
    Seq<decimal_constant,     Opt<complex_suffix>, Opt<integer_suffix>, Opt<complex_suffix>>,
    Seq<hexadecimal_constant, Opt<complex_suffix>, Opt<integer_suffix>, Opt<complex_suffix>>,
    Seq<binary_constant,      Opt<complex_suffix>, Opt<integer_suffix>, Opt<complex_suffix>>,
    Seq<octal_constant,       Opt<complex_suffix>, Opt<integer_suffix>, Opt<complex_suffix>>
  >;

  return integer_constant::match(a, b);
}
```
