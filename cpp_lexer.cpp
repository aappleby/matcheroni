#include "Matcheroni.h"

// from the N4950 C++23 draft spec

/*
typedef-name:
  identifier
  simple-template-id

namespace-name:
  identifier
  namespace-alias

namespace-alias:
  identifier

class-name:
  identifier
  simple-template-id

enum-name:
  identifier

template-name:
  identifier

n-char: one of
  any member of the translation character set except the u+007d right curly bracket or new-line character

n-char-sequence :
  n-char
  n-char-sequence n-char

named-universal-character:
  \N{ n-char-sequence }

hex-quad:
  hexadecimal-digit hexadecimal-digit hexadecimal-digit hexadecimal-digit

simple-hexadecimal-digit-sequence :
  hexadecimal-digit
  simple-hexadecimal-digit-sequence hexadecimal-digit

universal-character-name :
  \u hex-quad
  \U hex-quad hex-quad
  \u{ simple-hexadecimal-digit-sequence }
  named-universal-character

preprocessing-token:
  header-name
  import-keyword
  module-keyword
  export-keyword
  identifier
  pp-number
  character-literal
  user-defined-character-literal
  string-literal
  user-defined-string-literal
  preprocessing-op-or-punc
  each non-whitespace character that cannot be one of the above

token:
  identifier
  keyword
  literal
  operator-or-punctuator

header-name:
  < h-char-sequence >
  " q-char-sequence "

h-char-sequence :
  h-char
  h-char-sequence h-char

h-char:
  any member of the translation character set except new-line and u+003e greater-than sign

q-char-sequence :
  q-char
  q-char-sequence q-char

q-char:
  any member of the translation character set except new-line and u+0022 quotation mark

pp-number:
  digit
  . digit
  pp-number identifier-continue
  pp-number ’ digit
  pp-number ’ nondigit
  pp-number e sign
  pp-number E sign
  pp-number p sign
  pp-number P sign
  pp-number .

identifier:
  identifier-start
  identifier identifier-continue

identifier-start:
  nondigit
  an element of the translation character set with the Unicode property XID_Start

identifier-continue :
  digit
  nondigit
  an element of the translation character set with the Unicode property XID_Continue

nondigit: one of
  a b c d e f g h i j k l m
  n o p q r s t u v w x y z
  A B C D E F G H I J K L M
  N O P Q R S T U V W X Y Z _

digit: one of
  0 1 2 3 4 5 6 7 8 9

keyword:
  any identifier listed in Table 5
  import-keyword
  module-keyword
  export-keyword

preprocessing-op-or-punc :
  preprocessing-operator
  operator-or-punctuator

preprocessing-operator: one of
  # ## %: %:%:

operator-or-punctuator: one of
  { } [ ] ( )
  <: :> <% %> ; : ...
  ? :: . .* -> ->* ~
  ! + - * / % ^ & |
  = += -= *= /= %= ^= &= |=
  == != < > <= >= <=> && ||
  << >> <<= >>= ++ -- ,
  and or xor not bitand bitor compl
  and_eq or_eq xor_eq not_eq

literal:
  integer-literal
  character-literal
  floating-point-literal
  string-literal
  boolean-literal
  pointer-literal
  user-defined-literal

integer-literal:
  binary-literal integer-suffixopt
  octal-literal integer-suffixopt
  decimal-literal integer-suffixopt
  hexadecimal-literal integer-suffixopt

binary-literal:
  0b binary-digit
  0B binary-digit
  binary-literal ’opt binary-digit

octal-literal:
  0
  octal-literal ’opt octal-digit

decimal-literal:
  nonzero-digit
  decimal-literal ’opt digit

hexadecimal-literal:
  hexadecimal-prefix hexadecimal-digit-sequence

binary-digit: one of
  0 1

octal-digit: one of
  0 1 2 3 4 5 6 7

nonzero-digit: one of
  1 2 3 4 5 6 7 8 9

hexadecimal-prefix: one of
  0x 0X

hexadecimal-digit-sequence :
  hexadecimal-digit
  hexadecimal-digit-sequence ’opt hexadecimal-digit

hexadecimal-digit: one of
  0 1 2 3 4 5 6 7 8 9
  a b c d e f
  A B C D E F

integer-suffix:
  unsigned-suffix long-suffixopt
  unsigned-suffix long-long-suffixopt
  unsigned-suffix size-suffixopt
  long-suffix unsigned-suffixopt
  long-long-suffix unsigned-suffixopt
  size-suffix unsigned-suffixopt

unsigned-suffix: one of
  u U

long-suffix: one of
  l L

long-long-suffix: one of
  ll LL

size-suffix: one of
  z Z

character-literal:
  encoding-prefixopt ’ c-char-sequence ’

encoding-prefix: one of
  u8 u U L

c-char-sequence :
  c-char
  c-char-sequence c-char

c-char:
  basic-c-char
  escape-sequence
  universal-character-name

basic-c-char:
  any member of the translation character set except the u+0027 apostrophe,
  u+005c reverse solidus, or new-line character

escape-sequence :
  simple-escape-sequence
  numeric-escape-sequence
  conditional-escape-sequence

simple-escape-sequence :
  \ simple-escape-sequence-char

simple-escape-sequence-char: one of
  ’ " ? \ a b f n r t v

numeric-escape-sequence :
  octal-escape-sequence
  hexadecimal-escape-sequence

simple-octal-digit-sequence :
  octal-digit
  simple-octal-digit-sequence octal-digit

octal-escape-sequence :
  \ octal-digit
  \ octal-digit octal-digit
  \ octal-digit octal-digit octal-digit
  \o{ simple-octal-digit-sequence }

hexadecimal-escape-sequence :
  \x simple-hexadecimal-digit-sequence
  \x{ simple-hexadecimal-digit-sequence }

conditional-escape-sequence :
  \ conditional-escape-sequence-char

conditional-escape-sequence-char:
  any member of the basic character set that is not an octal-digit, a simple-escape-sequence-char, or the
  characters N, o, u, U, or x

floating-point-literal:
  decimal-floating-point-literal
  hexadecimal-floating-point-literal

decimal-floating-point-literal:
  fractional-constant exponent-partopt floating-point-suffixopt
  digit-sequence exponent-part floating-point-suffixopt

hexadecimal-floating-point-literal:
  hexadecimal-prefix hexadecimal-fractional-constant binary-exponent-part floating-point-suffixopt
  hexadecimal-prefix hexadecimal-digit-sequence binary-exponent-part floating-point-suffixopt

fractional-constant:
  digit-sequenceopt . digit-sequence
  digit-sequence .

hexadecimal-fractional-constant:
  hexadecimal-digit-sequenceopt . hexadecimal-digit-sequence
  hexadecimal-digit-sequence .

exponent-part:
  e signopt digit-sequence
  E signopt digit-sequence

binary-exponent-part:
  p signopt digit-sequence
  P signopt digit-sequence

sign: one of
  + -

digit-sequence:
  digit
  digit-sequence ’opt digit

floating-point-suffix: one of
  f l f16 f32 f64 f128 bf16 F L F16 F32 F64 F128 BF16

string-literal:
  encoding-prefixopt " s-char-sequenceopt "
  encoding-prefixopt R raw-string

s-char-sequence:
  s-char
  s-char-sequence s-char

s-char:
  basic-s-char
  escape-sequence
  universal-character-name

basic-s-char:
  any member of the translation character set except the u+0022 quotation mark,
  u+005c reverse solidus, or new-line character

raw-string:
  " d-char-sequenceopt ( r-char-sequenceopt ) d-char-sequenceopt "

r-char-sequence:
  r-char
  r-char-sequence r-char

r-char:
  any member of the translation character set, except a u+0029 right parenthesis followed by
  the initial d-char-sequence (which may be empty) followed by a u+0022 quotation mark

d-char-sequence :
  d-char
  d-char-sequence d-char

d-char:
  any member of the basic character set except:
  u+0020 space, u+0028 left parenthesis, u+0029 right parenthesis, u+005c reverse solidus,
  u+0009 character tabulation, u+000b line tabulation, u+000c form feed, and new-line

boolean-literal:
  false
  true

pointer-literal:
  nullptr

user-defined-literal:
  user-defined-integer-literal
  user-defined-floating-point-literal
  user-defined-string-literal
  user-defined-character-literal

user-defined-integer-literal:
  decimal-literal ud-suffix
  octal-literal ud-suffix
  hexadecimal-literal ud-suffix
  binary-literal ud-suffix

user-defined-floating-point-literal:
  fractional-constant exponent-partopt ud-suffix
  digit-sequence exponent-part ud-suffix
  hexadecimal-prefix hexadecimal-fractional-constant binary-exponent-part ud-suffix
  hexadecimal-prefix hexadecimal-digit-sequence binary-exponent-part ud-suffix

user-defined-string-literal:
  string-literal ud-suffix

user-defined-character-literal:
  character-literal ud-suffix

ud-suffix:
  identifier

*/
