#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Utilities.hpp"

using namespace matcheroni;

using blank   = Any<Atom<' ', '\t'>>;
using digit   = Range<'0', '9'>;
using alpha   = Range<'a', 'z', 'A', 'Z', '_', '_'>;
using ident   = Seq<alpha, Any<digit, alpha>>;
using comment = Seq<Lit<";">, Until<EOL>>;
using blank   = Seq<blank, EOL>;
using section = Seq<Atom<'['>, blank, ident, blank, Atom<']'>, blank, EOL>;
using token   = Any<NotAtom<' ', '\t'>>;
using value   = Seq<token, Any<Seq<blank, token>>>;
using keyval  = Seq<ident, blank, Atom<'='>, blank, value, blank, EOL>;
using line    = Oneof<section, keyval, comment, blank>;
using doc     = Any<line>;

bool parse_ini(const char* text) {
  auto span = to_span(text);

  TextContext ctx;
  auto tail = doc::match(ctx, span);
  return tail.is_valid();
}
