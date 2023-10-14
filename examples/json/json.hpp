#pragma once
#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"

matcheroni::TextSpan match_json(matcheroni::TextMatchContext& ctx, matcheroni::TextSpan body);

// JsonNodes are basically the same as TextNodes
struct JsonParseNode : public parseroni::NodeBase<JsonParseNode, char> {
  matcheroni::TextSpan as_text_span() const { return span; }
};

// Our nodes don't have anything to construct or destruct, so we turn
// constructors and destructors off during parsing.
struct JsonParseContext : public parseroni::NodeContext<JsonParseNode, false, false> {
  static int atom_cmp(char a, int b) { return (unsigned char)a - b; }
};

matcheroni::TextSpan parse_json(JsonParseContext& ctx, matcheroni::TextSpan body);
