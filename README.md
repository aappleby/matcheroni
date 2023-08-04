------
### Update 7/18/2023 - Matcheroni is getting a major rewrite plus a parsing framework I'm calling Parseroni. There will be a full live interactive tutorial as well. Please hold off on using this library until those are complete.

------
### TL;DR

------

# Matcheroni

[Matcheroni](https://github.com/aappleby/Matcheroni/blob/main/matcheroni/Matcheroni.hpp) is a minimalist, zero-dependency, single-header C++20 library for doing pattern matching using [Parsing Expression Grammars](https://en.wikipedia.org/wiki/Parsing_expression_grammar) (PEGs). PEGs are similar to regular expressions, but both more and less powerful.

[Parseroni](https://github.com/aappleby/Matcheroni/blob/main/matcheroni/Parseroni.hpp) is a companion single-header library that can capture the content of Matcheroni patterns and assemble them into concrete [parse trees](https://en.wikipedia.org/wiki/Parse_tree).

Matcheroni and Parseroni generate tiny, fast parsers that are easy to customize and integrate into your existing codebase.

A tutorial for building a JSON parser in Matcheroni+Parseroni can be found [here](https://aappleby.github.io/Matcheroni/tutorial)

# Building the Matcheroni examples and tests
Install [Ninja](https://ninja-build.org/) if you haven't already, then run ninja in the repo root.

See build.ninja for configuration options.

# It's all a bunch of templates? Really?
If you're familiar with C++ templates, you are probably concerned that this library will cause your binary size to explode and your compiler to grind to a halt.

I can assure you that that's not the case - binary sizes and compile times for even pretty large matchers are fine, though the resulting debug symbols are so enormous as to be useless.

# Built-in matchers
Since Matcheroni is based on Parsing Expression Grammars, it includes all the basic rules you'd expect:

| PEG | Regex | Matcheroni | Description |
| ----------- | ----------- | ----------- | ----------- |
| Header      | Title       |
| Paragraph   | Text        |



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

The JSON example contains a benchmark

# Caveats

Matcheroni requires C++20, which is a non-starter for some projects. There's not a lot I can do about that, as I'm heavily leveraging some newish template stuff that doesn't have any backwards-compatible equivalents.

Like parsing expression grammars, matchers are greedy - ```Seq<Some<Atom<'a'>>, Atom<'a'>>``` will _always_ fail as ```Some<Atom<'a'>>``` leaves no 'a's behind for the second ```Atom<'a'>``` to match.

Matcheroni does not implement any form of [packrat parsing](https://pdos.csail.mit.edu/~baford/packrat/icfp02/), though it could be added on top. Trying to do [operator-precedence parsing](https://en.wikipedia.org/wiki/Operator-precedence_parser) using the precedence-climbing method will be unbearably slow due to the huge number of recursive calls that don't end up matching anything.

Recursive matchers create recursive code that can explode your call stack.

Left-recursive matchers can get stuck in an infinite loop - this is true with most versions of Parsing Expression Grammars, it's a fundamental limitation of the algorithm.

# A Particularly Large Matcheroni Pattern

Here's the code I use to match C99 integers, plus a few additions from the C++ spec and the GCC extensions.

Note that it consists of 20 ```using``` declarations and the only actual "code" is ```return integer_constant::match(ctx, body);```

If you follow along in Appendix A of the [C99 spec](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf), you'll see it lines up quite closely.

```cpp
TextSpan match_int(TextMatchContext& ctx, TextSpan) {
  // clang-format off
  using digit                = Range<'0', '9'>;
  using nonzero_digit        = Range<'1', '9'>;

  using decimal_constant     = Seq<nonzero_digit, Any<ticked<digit>>>;

  using hexadecimal_prefix         = Oneof<Lit<"0x">, Lit<"0X">>;
  using hexadecimal_digit          = Range<'0','9','a','f','A','F'>;
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

  return integer_constant::match(ctx, body);
  // clang-format on
}
```
