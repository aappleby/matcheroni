#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Utilities.hpp"

using namespace matcheroni;

using ws      = Any<Atoms<' ', '\t'>>;
using digit   = Range<'0', '9'>;
using alpha   = Ranges<'a', 'z', 'A', 'Z', '_', '_'>;

using ident   = Seq<alpha, Any<digit, alpha>>;
using token   = Any<NotAtoms<' ', '\t'>>;
using value   = Seq<token, Any<Seq<ws, token>>>;

using section = Seq<Atom<'['>, ws, ident, ws, Atom<']'>, ws, EOL>;
using keyval  = Seq<ident, ws, Atom<'='>, ws, value, ws, EOL>;
using comment = Seq<Lit<";">, Until<EOL>>;
using blank   = Seq<ws, EOL>;

using line    = Oneof<section, keyval, comment, blank>;
using doc     = Any<line>;

bool parse_ini(const char* text) {
  auto span = utils::to_span(text);

  TextMatchContext ctx;
  auto tail = doc::match(ctx, span);
  return tail.is_valid();
}
