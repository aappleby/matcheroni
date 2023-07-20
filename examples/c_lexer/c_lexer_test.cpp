// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include "matcheroni/Utilities.hpp"

#include "examples/c_lexer/CLexer.hpp"

using namespace matcheroni;

//------------------------------------------------------------------------------
// FIXME - Is there anything we need to cover here that isn't already covered
// by matcheroni_test?

// FIXME - yeah we need a raw string backreference test :P

/*
void test_match_string() {
  const char* text1 = "asdfasdf \"Hello World\" 123456789";
  matcheroni_assert("\"Hello World\"" == get_first_match(text1, match_string));

  const char* text2 = "asdfasdf \"Hello\nWorld\" 123456789";
  matcheroni_assert("\"Hello\nWorld\"" == get_first_match(text2, match_string));

  const char* text3 = "asdfasdf \"Hello\\\"World\" 123456789";
  matcheroni_assert("\"Hello\\\"World\"" == get_first_match(text3, match_string));

  const char* text4 = "asdfasdf \"Hello\\\\World\" 123456789";
  matcheroni_assert("\"Hello\\\\World\"" == get_first_match(text4, match_string));

  printf("test_match_string() pass\n");
}
*/

const char* some_text =
R"(
#include <stdio.h>

int main(int argc, char** argv) {
  printf("Hello World\n");
  return 0;
}
)";

int main(int argc, char** argv) {

  std::string raw_text = some_text;
  raw_text.push_back(0);

  CLexer lexer;
  lexer.lex(to_span(raw_text));

  for (auto& l : lexer.tokens) {
    l.dump();
    printf("\n");
  }

  return 0;
}
