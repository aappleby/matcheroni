#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Utilities.hpp"

using namespace matcheroni;

using ws      = Any<Atom<' ', '\t'>>;
using digit   = Range<'0', '9'>;
using alpha   = Range<'a', 'z', 'A', 'Z', '_', '_'>;
using ident   = Seq<alpha, Any<digit, alpha>>;
using comment = Seq<Lit<";">, Until<EOL>>;
using blank   = Seq<ws, EOL>;
using section = Seq<Atom<'['>, ws, ident, ws, Atom<']'>, ws, EOL>;
using token   = Any<NotAtom<' ', '\t'>>;
using value   = Seq<token, Any<Seq<ws, token>>>;
using keyval  = Seq<ident, ws, Atom<'='>, ws, value, ws, EOL>;
using line    = Oneof<section, keyval, comment, blank>;
using doc     = Any<line>;


bool parse_ini(const char* text) {
  auto span = to_span(text);

  TextContext ctx;
  auto tail = doc::match(ctx, span);
  return tail.is_valid();
}
