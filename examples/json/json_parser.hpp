#pragma once
#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"

// JsonNodes are basically the same as TextNodes
struct JsonNode : public matcheroni::NodeBase<JsonNode, char> {
  matcheroni::TextSpan as_text() const { return span; }
  const char* text_head() const { return span.begin; }
  const char* text_tail() const { return span.end; }
};

// Our nodes don't have anything to construct or destruct, so we turn
// constructors and destructors off during parsing.
struct JsonContext : public matcheroni::NodeContext<JsonNode, false, false> {
  static int atom_cmp(char a, int b) { return (unsigned char)a - b; }
};

matcheroni::TextSpan parse_json(JsonContext& ctx, matcheroni::TextSpan body);
