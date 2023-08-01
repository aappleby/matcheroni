#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"
#include "matcheroni/Utilities.hpp"

#include <stdio.h>

using namespace matcheroni;
using namespace parseroni;

// Our base node type is the same as TextParseNode, with the addition of a
// sum() method.
struct JsonNode : public NodeBase<JsonNode, char> {
  TextSpan as_text() const { return span; }

  virtual double sum() {
    double result = 0;
    for (auto n = child_head; n; n = n->node_next) {
      result += n->sum();
    }
    return result;
  }
};

// We'll specialize JsonNode for numerical values by overriding init() to also
// convert the matched text to a double.
struct NumberNode : public JsonNode {
  void init(const char* match_name, TextSpan span, uint64_t flags) {
    JsonNode::init(match_name, span, flags);
    value = atof(span.begin);
  }

  virtual double sum() {
    return value;
  }

  double value = 0;
};

// And our context provides atom_cmp() and sum(). NodeContext<> handles the
// required checkpoint()/rewind() methods.
struct JsonContext : public NodeContext<JsonNode> {
  static int atom_cmp(char a, int b) {
    return (unsigned char)a - b;
  }

  double sum() {
    double result = 0;
    for (auto n = top_head; n; n = n->node_next) {
      result += n->sum();
    }
    return result;
  }
};

//------------------------------------------------------------------------------

struct JsonParser {
  // Matches any JSON number
  using sign      = Atom<'+', '-'>;
  using digit     = Range<'0', '9'>;
  using onenine   = Range<'1', '9'>;
  using digits    = Some<digit>;
  using integer   = Seq<Opt<Atom<'-'>>, Oneof<Seq<onenine, digits>, digit>>;
  using fraction  = Seq<Atom<'.'>, digits>;
  using exponent  = Seq<Atom<'e', 'E'>, Opt<sign>, digits>;
  using number    = Seq<integer, Opt<fraction>, Opt<exponent>>;

  // Matches a JSON string that can contain valid escape characters
  using space     = Some<Atom<' ', '\n', '\r', '\t'>>;
  using hex       = Range<'0','9','a','f','A','F'>;
  using escape    = Oneof<Charset<"\"\\/bfnrt">, Seq<Atom<'u'>, Rep<4, hex>>>;
  using character = Oneof<
    Seq<Not<Atom<'"'>>, Not<Atom<'\\'>>, Range<0x0020, 0x10FFFF>>,
    Seq<Atom<'\\'>, escape>
  >;
  using string = Seq<Atom<'"'>, Any<character>, Atom<'"'>>;

  // Matches the three reserved JSON keywords
  using keyword = Oneof<Lit<"true">, Lit<"false">, Lit<"null">>;

  // Matches a comma-delimited list with embedded whitespace
  template <typename P>
  using list = Seq<P, Any<Seq<Opt<space>, Atom<','>, Opt<space>, P>>>;

  // Matches any valid JSON value
  static TextSpan match_value(JsonContext& ctx, TextSpan body) {
    return Oneof<

      // **********
      // This Capture<> will now create NumberNodes
      Capture<"number",  number,  NumberNode>,
      // **********

      Capture<"string",  string,  JsonNode>,
      Capture<"array",   array,   JsonNode>,
      Capture<"object",  object,  JsonNode>,
      Capture<"keyword", keyword, JsonNode>
    >::match(ctx, body);
  }
  using value = Ref<match_value>;

  // Matches bracket-delimited lists of JSON values
  using array =
  Seq<
    Atom<'['>,
    Opt<space>,
    Opt<list<value>>,
    Opt<space>,
    Atom<']'>
  >;

  // Matches a key:value pair where 'key' is a string and 'value' is a JSON value.
  using pair =
  Seq<
    Capture<"key", string, JsonNode>,
    Opt<space>,
    Atom<':'>,
    Opt<space>,
    Capture<"value", value, JsonNode>
  >;

  // Matches a curly-brace-delimited list of key:value pairs.
  using object =
  Seq<
    Atom<'{'>,
    Opt<space>,
    Opt<list<Capture<"member", pair, JsonNode>>>,
    Opt<space>,
    Atom<'}'>
  >;

  // Matches any valid JSON document
  static TextSpan match(JsonContext& ctx, TextSpan body) {
    return Seq<Opt<space>, value, Opt<space>>::match(ctx, body);
  }
};

int main(int argc, char** argv) {
  if (argc < 2) exit(-1);

  std::string input = utils::read(argv[1]);
  TextSpan text = utils::to_span(input);

  JsonContext ctx;
  TextSpan tail = JsonParser::match(ctx, text);

  printf("Sum of number nodes: %f\n", ctx.sum());
  printf("\n");
  utils::print_summary(ctx, text, tail, 50);

  return tail.is_valid() ? 0 : -1;
}
