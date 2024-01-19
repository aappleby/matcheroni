#include <stdio.h>
#include <span>
#include <string.h>

using text_span = std::span<const char>;

struct Context {

  Context(text_span _span) : span(_span) {}
  Context(const char* text) : span(text, text + strlen(text)) {}

  text_span span;

  template<typename pattern>
  text_span take() {
    return text_span();
  }
};

struct identifier {};
struct whitespace {};

void blah() {

  Context ctx("Hello World");

  auto foo1 = ctx.take<identifier>();
  auto foo2 = ctx.take<whitespace>();
  auto foo3 = ctx.take<identifier>();

}

// pattern span context


// pattern + span + context -> head / tail

// tail = context.match<pattern>();
// tail = pattern::match(context);

// tail = pattern::match(context, span);

// head = take<pattern>(context, span);
