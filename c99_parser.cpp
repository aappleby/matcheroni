#include "Matcheroni.h"
#include <assert.h>
#include <stdio.h>
#include <memory.h>
#include "c_lexer.h"
#include "Node.h"

using namespace matcheroni;

const char* text = R"(
#include <stdio.h>

int main(int argc, char** argv) {
  printf("Hello World!\n");
  return 0;
}
)";

//------------------------------------------------------------------------------

int test_c99_peg() {
  printf("Hello World\n");

  auto text_a = text;
  auto text_b = text + strlen(text);

  auto cursor_a = text_a;
  auto cursor_b = text_a;

  NodeList tokens;

  cursor_a = cursor_b;
  cursor_b = match_preproc(cursor_b, text_b, nullptr);
  auto preproc = new Node(TOK_PREPROC, cursor_a, cursor_b);

  cursor_a = cursor_b;
  cursor_b = match_space(cursor_b, text_b, nullptr);
  auto space = new Node(TOK_SPACE, cursor_a, cursor_b);

  cursor_a = cursor_b;
  cursor_b = match_header_name(cursor_b, text_b, nullptr);
  auto path = new Node(TOK_HEADER_NAME, cursor_a, cursor_b);

  cursor_a = cursor_b;
  cursor_b = match_newline(cursor_b, text_b, nullptr);
  auto newline = new Node(TOK_NEWLINE, cursor_a, cursor_b);

  tokens.append(preproc);
  tokens.append(space);
  tokens.append(path);
  tokens.append(newline);
  tokens.sanity(text_a, text_b);

  auto root = new Node(TOK_ROOT, preproc->span_a, path->span_b);
  root->append(preproc);
  root->append(path);
  root->sanity();

  printf("text_a         %p\n", text_a);
  printf("preproc span_a %p\n", preproc->span_a);
  printf("preproc span_b %p\n", preproc->span_b);
  printf("space   span_a %p\n", space->span_a);
  printf("space   span_b %p\n", space->span_b);
  printf("path    span_a %p\n", path->span_a);
  printf("path    span_b %p\n", path->span_b);
  printf("newline span_a %p\n", newline->span_a);
  printf("newline span_b %p\n", newline->span_b);
  printf("text_b         %p\n", text_b);

  return 0;
}

//------------------------------------------------------------------------------
