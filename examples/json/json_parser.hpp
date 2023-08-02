#pragma once
#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"
#include <math.h>

// JsonNodes are basically the same as TextNodes
struct JsonNode : public parseroni::NodeBase<JsonNode, char> {
  matcheroni::TextSpan as_text() const { return span; }
};

// Our nodes don't have anything to construct or destruct, so we turn
// constructors and destructors off during parsing.
struct JsonContext : public parseroni::NodeContext<JsonNode, false, false> {
  static int atom_cmp(char a, int b) { return (unsigned char)a - b; }
};

matcheroni::TextSpan parse_json(JsonContext& ctx, matcheroni::TextSpan body);
