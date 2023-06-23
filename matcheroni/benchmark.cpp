#include "Matcheroni.h"

//#include <regex>
#include <chrono>
#include <string>

#ifdef MATCHERONI_USE_NAMESPACE
using namespace matcheroni;
#endif

int r_matches = 0;
int i_matches = 0;
int h_matches = 0;
int m_matches = 0;

double r_time = 0;
double m_time = 0;
double h_time = 0;
double i_time = 0;

//------------------------------------------------------------------------------
// Fill a buffer with 1 million random characters from "()abcdef", with a max
// paren nesting depth of 5.

void generate_buffer(std::string& buf) {
  const int buf_size = 1000000;
  buf.resize(buf_size);

  int depth = 0;
  int cursor = 0;
  while(cursor < buf_size) {
    auto c = "()abcdef"[rand() % 8];
    if (c == '(' && depth == 5) continue;
    if (c == ')' && depth == 0) continue;

    buf[cursor++] = c;
    if (c == '(') depth++;
    if (c == ')') depth--;
  }
}

//------------------------------------------------------------------------------

double timestamp_ms() {
  using clock = std::chrono::high_resolution_clock;
  using nano = std::chrono::nanoseconds;

  static bool init = false;
  static double origin = 0;

  auto now = clock::now().time_since_epoch();
  auto now_nanos = std::chrono::duration_cast<nano>(now).count();
  if (!origin) origin = now_nanos;

  return (now_nanos - origin) * 1.0e-6;
}

//------------------------------------------------------------------------------

/*
struct MatchingParens {
  static const char* match(const char* a, const char* b) {
    using pattern =
    Seq<
      Atom<'('>,
      Any<MatchingParens, NotAtom<')'>>,
      Atom<')'>
    >;
    return pattern::match(ctx, a, b);
  }
};
*/

__attribute__((noinline))
const char* matcheroni_match_parens(void* ctx, const char* a, const char* b) {
  using pattern =
  Seq<
    Atom<'('>,
    Any<Ref<matcheroni_match_parens>, NotAtom<')'>>,
    Atom<')'>
  >;
  return pattern::match(ctx, a, b);
}

//------------------------------------------------------------------------------

__attribute__((noinline))
static const char* recursive_matching_parens(void* ctx, const char* a, const char* b) {
  if (*a != '(') return nullptr;
  a++;
  while (a != b) {
    if (*a == ')') return a + 1;
    else if (*a == '(') {
      if (auto end = recursive_matching_parens(ctx, a, b)) {
        a = end;
      }
      else {
        return nullptr;
      }
    }
    else {
      a++;
    }
  }
  return nullptr;
}

__attribute__((noinline))
static const char* nonrecursive_matching_parens(void* ctx, const char* a, const char* b) {
  if (*a != '(') return nullptr;
  a++;
  int depth = 1;
  while (a != b) {
    if (*a == ')') {
      depth--;
      if (depth == 0) return a + 1;
    }
    else if (*a == '(') {
      depth++;
    }
    a++;
  }
  return nullptr;
}

//------------------------------------------------------------------------------

void benchmark_paired_parens() {
  std::string buf;
  generate_buffer(buf);

  printf("Warmup reps");
  const int reps = 10;
  for (int rep = 0; rep < reps; rep++) {
    if (rep == reps - 1) {
      printf("\n");
      printf("Benchmarking...\n\n");
      r_time = 0;
      m_time = 0;
      h_time = 0;
      i_time = 0;
      r_matches = 0;
      i_matches = 0;
      h_matches = 0;
      m_matches = 0;
    }
    else {
      printf(".");
      fflush(stdout);
    }

    // Straightforward handwritten matcher
    {
      const char* lparen = 0;

      h_time -= timestamp_ms();
      const char* a = buf.data();
      const char* b = buf.data() + buf.size();
      while(a != b) {
        /*
        auto end1 = recursive_matching_parens(a, b);
        auto end2 = nonrecursive_matching_parens(a, b);
        auto end3 = unrolled_matcheroni_matching_parens(a, b);
        auto end4 = MatchingParens::match(ctx, a, b);

        if (end1 != end2 || end2 != end3 || end3 != end4) {
          printf("mismatch?\n");
          end3 = unrolled_matcheroni_matching_parens(a, b);
        }
        */

        if (auto end = recursive_matching_parens(nullptr, a, b)) {
          h_matches++;
          a = end;
        }
        else {
          a++;
        }
      }
      h_time += timestamp_ms();
    }

    // Matcheroni matcher
    {
      m_time -= timestamp_ms();
      const char* a = buf.data();
      const char* b = buf.data() + buf.size();
      while(a != b) {
        if (m_matches == 669) {
          int x = 1;
          x++;
        }

        if (auto end = matcheroni_match_parens(nullptr, a, b)) {
          m_matches++;
          a = end;
        }
        else {
          a++;
        }
      }
      m_time += timestamp_ms();
    }
  }
}

//------------------------------------------------------------------------------

void benchmark_paren_letters() {
  std::string buf;
  generate_buffer(buf);

  printf("Warmup reps");
  const int reps = 10;
  for (int rep = 0; rep < reps; rep++) {
    if (rep == reps - 1) {
      printf("\n");
      printf("Benchmarking...\n\n");
      r_time = 0;
      m_time = 0;
      h_time = 0;
      i_time = 0;
      r_matches = 0;
      i_matches = 0;
      h_matches = 0;
      m_matches = 0;
    }
    else {
      printf(".");
      fflush(stdout);
    }

#if 0
    // Naive use of std::regex that calls regex_search() to find each match.
    {
      static std::regex paren_match_regex(R"(\([^()]+\))");

      r_time -= timestamp_ms();
      const char* a = buf.data();
      const char* b = buf.data() + buf.size();
      while(*a) {
        std::cmatch m;
        if (std::regex_search(a, m, paren_match_regex)) {
          r_matches++;
          auto p = m.position();
          auto l = m.length();
          a = a + p + l;
        }
        else {
          a++;
        }
      }
      r_time += timestamp_ms();
    }
#endif

#if 0
    // Better use of std::regex via regex_iterator
    {
      static std::regex paren_match_regex(R"(\([^()]+\))");
      std::cregex_iterator it (buf.data(), buf.data() + buf.size(), paren_match_regex);
      std::cregex_iterator end;

      i_time -= timestamp_ms();
      while (it != end) {
        i_matches++;
        ++it;
      }
      i_time += timestamp_ms();
    }
#endif

#if 1
    // Straightforward handwritten matcher
    {
      const char* lparen = 0;

      h_time -= timestamp_ms();
      const char* a = buf.data();
      const char* b = buf.data() + buf.size();
      while(a != b) {
        if (*a == '(') {
          lparen = a;
        } else if (*a == ')') {
          if (lparen && ((a - lparen) > 1)) {
            h_matches++;
          }
          lparen = nullptr;
        }
        a++;
      }
      h_time += timestamp_ms();
    }
#endif

#if 1
    // Matcheroni matcher
    {
      using matcher = Seq<Atom<'('>, Some<NotAtom<'(', ')'>>, Atom<')'>>;

      m_time -= timestamp_ms();
      const char* a = buf.data();
      const char* b = buf.data() + buf.size();
      while(a != b) {
        if (auto end = matcher::match(nullptr, a, b)) {
          m_matches++;
          a = end;
        }
        else {
          a++;
        }
      }
      m_time += timestamp_ms();
    }
#endif
  }
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {

  /*
  std::string temp = "(a)(((((";

  const char* a = temp.data();
  const char* b = a + temp.size();

  while(a != b) {
    if (auto end = MatchingParens::match(ctx, a, b)) {
      a = end;
    }
    else {
      a++;
    }
  }
  exit(0);
  */

  //benchmark_paired_parens();
  benchmark_paren_letters();

  printf("std::regex match count %d\n", r_matches);
  printf("std::regex elapsed time %f ms\n", r_time);
  printf("\n");

  printf("std::regex_iterator match count %d\n", i_matches);
  printf("std::regex_iterator elapsed time %f ms\n", i_time);
  printf("\n");

  printf("Handwritten match count %d\n", h_matches);
  printf("Handwritten elapsed time %f ms\n", h_time);
  printf("\n");

  printf("Matcheroni match count %d\n", m_matches);
  printf("Matcheroni elapsed time %f ms\n", m_time);
  printf("\n");

  printf("Matcheroni is %f times faster than std::regex_search\n", r_time / m_time);
  printf("Matcheroni is %f times faster than std::regex_iterator\n", i_time / m_time);
  printf("Matcheroni is %f times faster than handwritten\n", h_time / m_time);

  return 0;
}

//------------------------------------------------------------------------------
