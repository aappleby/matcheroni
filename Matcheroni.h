#ifndef __MATCHERONI_H__
#define __MATCHERONI_H__

namespace matcheroni {

//------------------------------------------------------------------------------
// Matcheroni is based on building trees of "matcher" functions. A matcher takes
// a raw C string as input and returns either the endpoint of the match if
// found, or nullptr if a match was not found.
// Matchers must also handle null pointers and empty strings.

typedef const char*(*matcher)(const char*);

//------------------------------------------------------------------------------
// The most fundamental unit of matching is a single character. For convenience,
// we implement the Char matcher so that it can handle small sets of characters
// and allows Char<> to match any nonzero character.

// Examples:
// Char<'a'>::match("abcd") == "bcd"
// Char<'a','c'>::match("cdef") == "def"
// Char<>::match("z123") == "123"
// Char<>::match("") == nullptr

template <char... rest>
struct Char;

template <char C1, char... rest>
struct Char<C1, rest...> {
  static const char* match(const char* cursor) {
    // We should never explicitly match the null terminator, as we can't
    // advance past it.
    static_assert(C1 != 0);
    if (!cursor || !cursor[0]) return nullptr;
    if (cursor && cursor[0] == C1) return cursor + 1;
    return Char<rest...>::match(cursor);
  }
};

template <char C1>
struct Char<C1> {
  static const char* match(const char* cursor) {
    // We should never explicitly match the null terminator, as we can't
    // advance past it.
    static_assert(C1 != 0);
    if (!cursor || !cursor[0]) return nullptr;
    if (cursor && cursor[0] == C1) return cursor + 1;
    return nullptr;
  }
};

template <>
struct Char<> {
  static const char* match(const char* cursor) {
    if (!cursor || !cursor[0]) return nullptr;
    if (!cursor) return nullptr;
    if (!cursor[0]) return nullptr;
    return cursor + 1;
  }
};

//------------------------------------------------------------------------------

struct NUL {
  static const char* match(const char* cursor) {
    if (!cursor) return nullptr;
    return cursor[0] == 0 ? cursor : nullptr;
  }
};

//------------------------------------------------------------------------------
// Matches LF and NUL, but does not advance past it.

struct EOL {
  static const char* match(const char* cursor) {
    if (!cursor) return nullptr;
    if (cursor[0] == 0) return cursor;
    if (cursor[0] == '\n') return cursor;
    return nullptr;
  }
};

//------------------------------------------------------------------------------
// Matches backslash+LF, which is used as the line-continuation marker in C

/*
struct CONT {
  static const char* match(const char* cursor) {
    if (!cursor) return nullptr;
    if (cursor[0] != '\\') return nullptr;
    if (cursor[1] != '\n') return nullptr;
    return cursor + 2;
  }
};
*/

//------------------------------------------------------------------------------
// Consumes any character that is not LF or NUL

struct NotEOL {
  static const char* match(const char* cursor) {
    if (!cursor) return nullptr;

    // A CONT is _not_ EOL

    if ((cursor[0] == '\\') && (cursor[1] == '\r') && (cursor[2] == '\n')) {
      return cursor + 3;
    }


    if ((cursor[0] == '\\') && (cursor[1] == '\n')) {
      return cursor + 2;
    }

    if (cursor[0] == 0) return nullptr;
    if (cursor[0] == '\n') return nullptr;
    return cursor + 1;
  }
};

//------------------------------------------------------------------------------
// The 'Seq' matcher succeeds if all of its sub-matchers succeed in order.

// Examples:
// Seq<Char<'a'>, Char<'b'>::match("abcd") == "cd"

template<typename M1, typename... rest>
struct Seq {
  static const char* match(const char* cursor) {
    if (!cursor || !cursor[0]) return nullptr;
    cursor = M1::match(cursor);
    if (!cursor) return nullptr;
    return Seq<rest...>::match(cursor);
  }
};

template<typename M1>
struct Seq<M1> {
  static const char* match(const char* cursor) {
    return M1::match(cursor);
  }
};

//------------------------------------------------------------------------------
// 'Oneof' returns the first match in a set of matchers, equivalent to (A|B|C)
// in regex.

// Examples:
// Oneof<Char<'a'>, Char<'b'>>::match("abcd") == "bcd"
// Oneof<Char<'a'>, Char<'b'>>::match("bcde") == "cde"

template <typename M1, typename... rest>
struct Oneof {
  static const char* match(const char* cursor) {
    if (auto end = M1::match(cursor)) return end;
    return Oneof<rest...>::match(cursor);
  }
};

template <typename M1>
struct Oneof<M1> {
  static const char* match(const char* cursor) {
    return M1::match(cursor);
  }
};

//------------------------------------------------------------------------------
// Zero-or-more patterns, equivalent to M* in regex.

// Examples:
// Any<Char<'a'>>::match("aaaab") == "b"
// Any<Char<'a'>>::match("bbbbc") == "bbbbc"

template<typename M>
struct Any {
  static const char* match(const char* cursor) {
    while(1) {
      auto end = M::match(cursor);
      if (!end) break;
      cursor = end;
    }
    return cursor;
  }
};

//------------------------------------------------------------------------------
// One-or-more patterns, equivalent to M+ in regex.

// Examples:
// Some<Char<'a'>>::match("aaaab") == "b"
// Some<Char<'a'>>::match("bbbbc") == nullptr

template<typename M>
struct Some {
  static const char* match(const char* cursor) {
    cursor = M::match(cursor);
    return cursor ? Any<M>::match(cursor) : nullptr;
  }
};

//------------------------------------------------------------------------------
// Optional patterns, equivalent to M? in regex.

// Opt<Char<'a'>>::match("abcd") == "bcd"
// Opt<Char<'a'>>::match("bcde") == "bcde"

template<typename M>
struct Opt {
  static const char* match(const char* cursor) {
    if (!cursor || !cursor[0]) return nullptr;
    if (auto end = M::match(cursor)) return end;
    return cursor;
  }
};

//------------------------------------------------------------------------------
// The 'and' predicate, which matches but does _not_ advance the cursor. Used
// for lookahead.

// And<Char<'a'>>::match("abcd") == "abcd"
// And<Char<'a'>>::match("bcde") == nullptr

template<typename M>
struct And {
  static const char* match(const char* cursor) {
    return M::match(cursor) ? cursor : nullptr;
  }
};

//------------------------------------------------------------------------------
// The 'not' predicate, the logical negation of the 'and' predicate.

// Not<Char<'a'>>::match("abcd") == nullptr
// Not<Char<'a'>>::match("bcde") == "bcde"

template<typename M>
struct Not {
  static const char* match(const char* cursor) {
    return M::match(cursor) ? nullptr : cursor;
  }
};

//------------------------------------------------------------------------------
// Char-not-in-set matcher, which is a bit faster than using
// Seq<Not<Char<...>>, Char<>>

template <char C1, char... rest>
struct NotChar {
  static const char* match(const char* cursor) {
    if (!cursor || !cursor[0]) return nullptr;
    if (cursor[0] == C1) return nullptr;
    return NotChar<rest...>::match(cursor);
  }
};

template <char C1>
struct NotChar<C1> {
  static const char* match(const char* cursor) {
    if (!cursor || !cursor[0]) return nullptr;
    if (cursor[0] == C1) return nullptr;
    return cursor + 1;
  }
};

//------------------------------------------------------------------------------
// Ranges of characters, equivalent to [a-z] in regex.

template<int A, int B>
struct Range {
  static const char* match(const char* cursor) {
    if(!cursor) return nullptr;
    if ((unsigned char)cursor[0] >= A &&
        (unsigned char)cursor[0] <= B) {
      return cursor + 1;
    }
    return nullptr;
  }
};

//------------------------------------------------------------------------------
// Repetition, equivalent to M{N} in regex.

template<int N, typename M>
struct Rep {
  static const char* match(const char* cursor) {
    for(auto i = 0; i < N; i++) {
      cursor = M::match(cursor);
      if (!cursor) return nullptr;
    }
    return cursor;
  }
};

//------------------------------------------------------------------------------
// Not a matcher, but a template helper that allows us to use strings as
// template arguments. The parameter behaves as a fixed-length character array
// that does ___NOT___ include the trailing null.

template<int N>
struct StringParam {
  constexpr StringParam(const char (&str)[N]) {
    for (int i = 0; i < N-1; i++) {
      value[i] = str[i];
    }
    //std::copy_n(str, N-1, value);
  }
  char value[N-1];
};

//------------------------------------------------------------------------------
// Matches string literals. Does ___NOT___ include the trailing null.

// Lit<"foo">::match("foobar") == "bar"

template<StringParam lit>
struct Lit {
  static const char* match(const char* cursor) {
    if(!cursor) return nullptr;
    if(!cursor[0]) return nullptr;
    for (auto i = 0; i < sizeof(lit.value); i++) {
      if (cursor[i] == 0) return nullptr;
      if (cursor[i] != lit.value[i]) return nullptr;
    }
    return cursor + sizeof(lit.value);
  }
};

//------------------------------------------------------------------------------
// Matches any non-NUL characters followed by the given pattern. The pattern is
// not consumed.

// Equivalent to Any<Seq<Not<M>,Char<>>>

template<typename M>
struct Until {
  static const char* match(const char* cursor) {
    while(cursor && *cursor) {
      if (M::match(cursor)) return cursor;
      cursor++;
    }
    return nullptr;
  }
};


//------------------------------------------------------------------------------
// Matches larger sets of chars, digraphs, and trigraphs packed into a string
// literal.

// Chars<"abcdef">::match("defg") == "efg"
// Digraphs<"aabbcc">::match("bbccdd") == "ccddee"
// Trigraphs<"abc123xyz">::match("123456") == "456"

template<StringParam chars>
struct Chars {
  static const char* match(const char* cursor) {
    if(!cursor) return nullptr;
    for (auto i = 0; i < sizeof(chars.value); i++) {
      if (cursor[0] == chars.value[i]) {
        return cursor + 1;
      }
    }
    return nullptr;
  }
};

template<StringParam chars>
struct Digraphs {
  static const char* match(const char* cursor) {
    if(!cursor) return nullptr;
    for (auto i = 0; i < sizeof(chars.value); i += 2) {
      if (cursor[0] == chars.value[i+0] &&
          cursor[1] == chars.value[i+1]) {
        return cursor + 2;
      }
    }
    return nullptr;
  }
};

template<StringParam chars>
struct Trigraphs {
  static const char* match(const char* cursor) {
    if(!cursor) return nullptr;
    for (auto i = 0; i < sizeof(chars.value); i += 3) {
      if (cursor[0] == chars.value[i+0] &&
          cursor[1] == chars.value[i+1] &&
          cursor[2] == chars.value[i+2]) {
        return cursor + 3;
      }
    }
    return nullptr;
  }
};

//------------------------------------------------------------------------------
// References to other matchers. Enables recursive matchers.

template<auto& F>
struct Ref {
  static const char* match(const char* cursor) {
    return F(cursor);
  }
};

//------------------------------------------------------------------------------

}; // namespace Matcheroni

#endif // #ifndef __MATCHERONI_H__
