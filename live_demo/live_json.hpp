#pragma once
#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"

struct JsonMatchContext : public matcheroni::TextMatchContext {
};

matcheroni::TextSpan match_json(JsonMatchContext& ctx, matcheroni::TextSpan body);

// JsonNodes are basically the same as TextNodes
struct JsonNode : public parseroni::NodeBase<JsonNode, char> {
  virtual ~JsonNode() {}
  matcheroni::TextSpan as_text_span() const { return span; }
};

struct JsonNumber  : public JsonNode {};
struct JsonString  : public JsonNode {};
struct JsonArray   : public JsonNode {};
struct JsonKeyVal  : public JsonNode {};
struct JsonObject  : public JsonNode {};
struct JsonKeyword : public JsonNode {};

// Our nodes don't have anything to construct or destruct, so we turn
// constructors and destructors off during parsing.
struct JsonParseContext : public parseroni::NodeContext<JsonNode, true, true> {
  static int atom_cmp(char a, int b) { return (unsigned char)a - b; }
};

matcheroni::TextSpan parse_json(JsonParseContext& ctx, matcheroni::TextSpan body);
