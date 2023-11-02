#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"
#include "matcheroni/Utilities.hpp"

using namespace matcheroni;
using namespace parseroni;

template<StringParam name, typename pattern>
using Cap = Capture<name, pattern, TextParseNode>;

// https://epage.github.io/blog/2023/07/winnow-0-5-the-fastest-rust-parser-combinator-library/

using space   = Any<Atoms<' ','\n'>>;
using comment = Seq<Atom<'#'>, Until<EOL>>;

using cooked_string =
Seq<
  Atom<'"'>,
  Until<Atom<'"'>>,
  Atom<'"'>
>;

using single_string =
Seq<
  Atom<'\''>,
  Until<Atom<'\''>>,
  Atom<'\''>
>;

using raw_string =
Seq<
  Lit<"\"\"\"">,
  Until<Lit<"\"\"\"">>,
  Lit<"\"\"\"">
>;

using string  = Oneof<raw_string, cooked_string, single_string>;

using number  = Some<Ranges<'0','9','_','_'>>;
using boolean = Oneof<Lit<"true">, Lit<"false">>;
using key     = Some<Ranges<'0','9','a','z','A','Z','_','_','-','-'>>;
// using date = ???

template<typename val, typename delim>
using delimited_list = Opt<Seq<val, Any<Seq<space, delim, space, val>>, space, Opt<delim>>>;

static TextSpan match_value(TextParseContext& ctx, TextSpan body);
using value = Ref<match_value>;

using key_or_str = Oneof<key, string>;
using path =
delimited_list<
  Cap<"path_el", key_or_str>,
  Atom<'.'>
>;

using table_header =
Seq<
  Atom<'['>,
  space,
  Cap<"path", path>,
  space,
  Atom<']'>
>;

using table_array_header =
Seq<
  Atom<'['>, Atom<'['>,
  space,
  Cap<"path", path>,
  space,
  Atom<']'>, Atom<']'>
>;

using key_value =
Seq<
  Cap<"key", key_or_str>,
  space,
  Atom<'='>,
  space,
  Cap<"value", value>
>;

using table_entries = Any<key_value>;

using table =
Seq<
  Cap<"header", table_header>,
  Cap<"entries", table_entries>
>;

using table_array =
Seq<
  table_array_header,
  table_entries
>;

using inline_table =
Seq<
  Atom<'{'>,
  space,
  delimited_list<
    Cap<"item", key_value>,
    Atom<','>
  >,
  space,
  Atom<'}'>
>;

using array =
Seq<
  Atom<'['>,
  space,
  delimited_list<
    value,
    Atom<','>
  >,
  space,
  Atom<']'>
>;

using expression =
Oneof<
  Cap<"pair",        key_value>,
  Cap<"table",       table>,
  Cap<"table_array", table_array>
>;

using tomlFile =
Any<Seq<
  space,
  expression,
  space
>>;

static TextSpan match_value(TextParseContext& ctx, TextSpan body) {
  using value =
  Oneof<
    Cap<"string", string>,
    Cap<"number", number>,
    Cap<"boolean", boolean>,
    /*date,*/
    Cap<"array", array>,
    Cap<"inline_table", inline_table>
  >;
  return value::match(ctx, body);
}

TextSpan match_toml(TextParseContext& ctx, TextSpan body) {
  return tomlFile::match(ctx, body);
}
