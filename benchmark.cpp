#include "Matcheroni.h"

#include <regex>
#include <chrono>
#include <string>

#ifdef MATCHERONI_USE_NAMESPACE
using namespace matcheroni;
#endif

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

int main(int argc, char** argv) {

  // Fill a buffer with 1 million random characters from "()abcdef"
  const int buf_size = 1000000;
  std::string buf;
  buf.resize(buf_size + 1);
  for (auto i = 0; i < buf_size; i++) {
    buf[i] = "()abcdef"[rand() % 8];
  }
  buf.back() = 0;

  // We will be searching for pairs of parens that contain only letters.
  std::regex paren_match_regex(R"(\([^()]+\))");

  printf("Warmup reps");
  const int reps = 10;
  for (int rep = 0; rep < reps; rep++) {
    if (rep == reps - 1) {
      printf("\n");
      printf("Benchmarking...\n\n");
    }
    else {
      printf(".");
      fflush(stdout);
    }

    double r_time_a, r_time_b;
    double m_time_a, m_time_b;
    double c_time_a, c_time_b;
    double i_time_a, i_time_b;

    // Naive use of std::regex that calls regex_search() to find each match.
    {
      const char* cursor = buf.c_str();
      int matches = 0;

      r_time_a = timestamp_ms();
      while(cursor[0]) {
        std::cmatch m;
        if (std::regex_search(cursor, m, paren_match_regex)) {
          matches++;
          auto p = m.position();
          auto l = m.length();
          cursor = cursor + p + l;
        }
        else {
          cursor++;
        }
      }
      r_time_b = timestamp_ms();

      if (rep == reps-1) {
        printf("std::regex match count %d\n", matches);
        printf("std::regex elapsed time %f ms\n", (r_time_b - r_time_a));
        printf("\n");
      }
    }

    // Better use of std::regex via regex_iterator
    {
      std::cregex_iterator it (buf.data(), buf.data() + buf_size, paren_match_regex);
      std::cregex_iterator end;
      int matches = 0;

      i_time_a = timestamp_ms();
      while (it != end) {
        matches++;
        ++it;
      }
      i_time_b = timestamp_ms();

      if (rep == reps-1) {
        printf("std::regex_iterator match count %d\n", matches);
        printf("std::regex_iterator elapsed time %f ms\n", (i_time_b - i_time_a));
        printf("\n");
      }
    }

    // Straightforward handwritten matcher
    {
      const char* cursor = buf.c_str();
      const char* lparen = 0;
      int matches = 0;

      c_time_a = timestamp_ms();
      while(cursor[0]) {
        if (*cursor == '(') {
          lparen = cursor;
        } else if (*cursor == ')') {
          if (lparen && ((cursor - lparen) > 1)) {
            matches++;
          }
          lparen = nullptr;
        }
        cursor++;
      }
      c_time_b = timestamp_ms();

      if (rep == reps-1) {
        printf("Handwritten match count %d\n", matches);
        printf("Handwritten elapsed time %f ms\n", (c_time_b - c_time_a));
        printf("\n");
      }
    }

    // Matcheroni matcher
    {
      using matcher = Seq<Atom<'('>, Some<NotAtom<'(', ')'>>, Atom<')'>>;
      const char* cursor = buf.c_str();
      int matches = 0;

      m_time_a = timestamp_ms();
      while(cursor[0]) {
        if (auto end = matcher::match(cursor)) {
          matches++;
          cursor = end;
        }
        else {
          cursor++;
        }
      }
      m_time_b = timestamp_ms();

      if (rep == reps-1) {
        printf("Matcheroni match count %d\n", matches);
        printf("Matcheroni elapsed time %f ms\n", (m_time_b - m_time_a));
        printf("\n");
      }
    }

    if (rep == reps-1) {
      printf("Matcheroni is %f times faster than std::regex\n", (r_time_b - r_time_a) / (m_time_b - m_time_a));
      printf("Matcheroni is %f times faster than std::regex_iterator\n", (i_time_b - i_time_a) / (m_time_b - m_time_a));
      printf("Matcheroni is %f times faster than handwritten\n", (c_time_b - c_time_a) / (m_time_b - m_time_a));
    }
  }
}
