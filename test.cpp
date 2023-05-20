#include "examples.h"

#include <stdio.h>
#include <assert.h>
#include <string>

typedef const char* (*matcher)(const char*);

std::string get_first_match(const char* text, matcher m) {
  for (; text && text[0]; text++) {
    if (auto end = m(text)) return std::string(text, end);
  }
  return "";
}

void test_match_string() {
  const char* text1 = "asdfasdf \"Hello World\" 123456789";
  assert("\"Hello World\"" == get_first_match(text1, match_string_literal));

  const char* text2 = "asdfasdf \"Hello\nWorld\" 123456789";
  assert("\"Hello\nWorld\"" == get_first_match(text2, match_string_literal));

  const char* text3 = "asdfasdf \"Hello\\\"World\" 123456789";
  assert("\"Hello\\\"World\"" == get_first_match(text3, match_string_literal));

  const char* text4 = "asdfasdf \"Hello\\\\World\" 123456789";
  assert("\"Hello\\\\World\"" == get_first_match(text4, match_string_literal));

  printf("test_match_string() pass\n");
}

int main(int argc, char** argv) {
  printf("Matcheroni Test\n");
  test_match_string();
  return 0;
}
