// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#include <stdio.h>

#define REGEX_BENCHMARK_BASELINE
#define REGEX_BENCHMARK_BOOST
//#define REGEX_BENCHMARK_CTRE
#define REGEX_BENCHMARK_MATCHERONI
//#define REGEX_BENCHMARK_SRELL
#define REGEX_BENCHMARK_STD_REGEX


#ifdef REGEX_BENCHMARK_BOOST
#include <boost/regex.hpp>
#endif

#ifdef REGEX_BENCHMARK_CTRE
#include "ctre.hpp"
#endif

#ifdef REGEX_BENCHMARK_MATCHERONI
#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Utilities.hpp"
using namespace matcheroni;
#endif

#ifdef REGEX_BENCHMARK_SRELL
#include "srell.hpp"
#endif

#ifdef REGEX_BENCHMARK_STD_REGEX
#include <regex>
#endif

// Reference regexes (as C strings):
// Email "[\\w.+-]+@[\\w.-]+\\.[\\w.-]+"
// URI   "[\\w]+:\\/\\/[^\\/\\s?#]+[^\\s?#]+(?:\\?[^\\s#]*)?(?:#[^\\s]*)?"
// IP    "(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9])\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9])"

//------------------------------------------------------------------------------

#ifdef REGEX_BENCHMARK_BASELINE

void benchmark_baseline(const char* path) {
  std::string buf = utils::read(path);

  {
    int checksum = 0;
    double time = -utils::timestamp_ms();
    for (auto c : buf) {
      checksum = checksum * 0x1234567 ^ c;
    }
    time += utils::timestamp_ms();
    printf("Email: Checksum 0x%08x, time %f\n", checksum, time);
  }

  {
    int checksum = 0;
    double time = -utils::timestamp_ms();
    for (auto c : buf) {
      checksum = checksum * 0x7654321 ^ c;
    }
    time += utils::timestamp_ms();
    printf("Email: Checksum 0x%08x, time %f\n", checksum, time);
  }

  {
    int checksum = 0;
    double time = -utils::timestamp_ms();
    for (auto c : buf) {
      checksum = checksum * 0x123321 ^ c;
    }
    time += utils::timestamp_ms();
    printf("Email: Checksum 0x%08x, time %f\n", checksum, time);
  }
}

#endif

//------------------------------------------------------------------------------

#ifdef REGEX_BENCHMARK_MATCHERONI

TextMatchContext ctx;

template<typename P>
void benchmark_pattern(TextSpan body) {
  int matches = 0;
  double time = 0;

  time -= utils::timestamp_ms();
  while(body.begin != body.end) {
    auto tail = P::match(ctx, body);
    if (tail.is_valid()) {
      matches++;
      body.begin = tail.begin;
    }
    else {
      body.begin++;
    }
  }
  time += utils::timestamp_ms();

  printf("Match count %4d, time %f msec\n", matches, time);
  fflush(stdout);
}

using w = Ranges<'a','z', 'A','Z', '0','9', '_', '_'>;
using dot = Atom<'.'>;
using at = Atom<'@'>;
using plus = Atom<'+'>;
using minus = Atom<'-'>;

using matcheroni_email_pattern = Seq<
  Some<w, dot, plus, minus>,
  at,
  Some<w, minus>,
  dot,
  Some<w, dot, minus>
>;

using matcheroni_url_pattern =
Seq<
  Some<w>,
  Lit<"://">,
  Some<NotAtoms<'/',' ','\t','\n','?','#'>>,
  Any<NotAtoms<' ','\t','\n','?','#'>>,

  Opt<
    Seq<
      Atom<'?'>,
      Any<NotAtoms<' ','\t','\n','#'>>
    >
  >,
  Opt<
    Seq<
      Atom<'#'>,
      Any<NotAtoms<' ','\t','\n'>>
    >
  >
>;

using zero_to_255 = Oneof<
  Seq< Atom<'2'>, Atom<'5'>,      Range<'0', '5'> >,
  Seq< Atom<'2'>, Range<'0','4'>, Range<'0', '9'> >,
  Seq< Atom<'1'>, Range<'0','9'>, Range<'0', '9'> >,
  Seq<            Range<'0','9'>, Range<'0', '9'> >,
  Seq<                            Range<'0', '9'> >
>;

using matcheroni_ip4_pattern = Seq<
  Rep<3, Seq<zero_to_255, Atom<'.'>>>,
  zero_to_255
>;

void benchmark_matcheroni(const char* path) {
  std::string buf = utils::read(path);
  TextSpan body = utils::to_span(buf);

  printf("Email: ");
  benchmark_pattern<matcheroni_email_pattern>(body);

  printf("URL:   ");
  benchmark_pattern<matcheroni_url_pattern>(body);

  printf("IP4:   ");
  benchmark_pattern<matcheroni_ip4_pattern>(body);
}

#endif

//------------------------------------------------------------------------------

#ifdef REGEX_BENCHMARK_STD_REGEX

void benchmark_std_regex(const char* path) {
  std::string buf = utils::read(path);

  const char* regex_email = "[\\w.+-]+@[\\w.-]+\\.[\\w.-]+";
  const char* regex_url   = "[\\w]+:\\/\\/[^\\/\\s?#]+[^\\s?#]+(?:\\?[^\\s#]*)?(?:#[^\\s]*)?";
  const char* regex_ip4   = "(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9])\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9])";

  {
    static std::regex r(regex_email);
    std::cregex_iterator it (buf.data(), buf.data() + buf.size(), r);
    std::cregex_iterator end;

    int match_count = 0;
    double time = -utils::timestamp_ms();
    while (it != end) {
      match_count++;
      it++;
    }
    time += utils::timestamp_ms();
    printf("Email: Match count %4d, time %f\n", match_count, time);
  }

  {
    static std::regex r(regex_url);
    std::cregex_iterator it (buf.data(), buf.data() + buf.size(), r);
    std::cregex_iterator end;

    int match_count = 0;
    double time = -utils::timestamp_ms();
    while (it != end) {
      match_count++;
      it++;
    }
    time += utils::timestamp_ms();
    printf("URL:   Match count %4d, time %f\n", match_count, time);
  }

  {
    static std::regex r(regex_ip4);
    std::cregex_iterator it (buf.data(), buf.data() + buf.size(), r);
    std::cregex_iterator end;

    int match_count = 0;
    double time = -utils::timestamp_ms();
    while (it != end) {
      match_count++;
      it++;
    }
    time += utils::timestamp_ms();
    printf("IP4:   Match count %4d, time %f\n", match_count, time);
  }
}
#endif

//------------------------------------------------------------------------------

#ifdef REGEX_BENCHMARK_BOOST

void benchmark_boost_regex(const char* path) {
  std::string buf = utils::read(path);

  const char* regex_email = "[\\w.+-]+@[\\w.-]+\\.[\\w.-]+";
  const char* regex_url   = "[\\w]+:\\/\\/[^\\/\\s?#]+[^\\s?#]+(?:\\?[^\\s#]*)?(?:#[^\\s]*)?";
  const char* regex_ip4   = "(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9])\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9])";

  {
    static boost::regex r(regex_email);
    boost::cregex_iterator it (buf.data(), buf.data() + buf.size(), r);
    boost::cregex_iterator end;

    int match_count = 0;
    double time = -utils::timestamp_ms();
    while (it != end) {
      match_count++;
      it++;
    }
    time += utils::timestamp_ms();
    printf("Email: Match count %4d, time %f\n", match_count, time);
  }

  {
    static boost::regex r(regex_url);
    boost::cregex_iterator it (buf.data(), buf.data() + buf.size(), r);
    boost::cregex_iterator end;

    int match_count = 0;
    double time = -utils::timestamp_ms();
    while (it != end) {
      match_count++;
      it++;
    }
    time += utils::timestamp_ms();
    printf("URL:   Match count %4d, time %f\n", match_count, time);
  }

  {
    static boost::regex r(regex_ip4);
    boost::cregex_iterator it (buf.data(), buf.data() + buf.size(), r);
    boost::cregex_iterator end;

    int match_count = 0;
    double time = -utils::timestamp_ms();
    while (it != end) {
      match_count++;
      it++;
    }
    time += utils::timestamp_ms();
    printf("IP4:   Match count %4d, time %f\n", match_count, time);
  }
}
#endif

//------------------------------------------------------------------------------

#ifdef REGEX_BENCHMARK_CTRE
void benchmark_ctre(const char* path) {
  std::string buf = read(path);

  auto match_email = ctre::search<"[\\w\\.+\\-]+@[\\w\\.\\-]+\\.[\\w\\.\\-]+">;
  auto match_url = ctre::search<"[\\w]+://[^/\\s?#]+[^\\s?#]+(?:\\?[^\\s#]*)?(?:#[^\\s]*)?">;
  auto match_ip4 = ctre::search<"(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9])\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9])">;

  printf("CTRE email\n");
  {
    const char* cursor = buf.data();
    int match_count = 0;
    double time = 0;
    time -= timestamp_ms();
    while (auto r = match_email(cursor)) {
      match_count++;
      cursor = r.end();
    }
    time += timestamp_ms();
    printf("Email: Match count %4d, time %f\n", match_count, time);
  }

  {
    const char* cursor = buf.data();
    int match_count = 0;
    double time = 0;
    time -= timestamp_ms();
    while (auto r = match_url(cursor)) {
      match_count++;
      cursor = r.end();
    }
    time += timestamp_ms();
    printf("URL:   Match count %4d, time %f\n", match_count, time);
  }

  {
    const char* cursor = buf.data();
    int match_count = 0;
    double time = 0;
    time -= timestamp_ms();
    while (auto r = match_ip4(cursor)) {
      match_count++;
      cursor = r.end();
    }
    time += timestamp_ms();
    printf("IP4:   Match count %4d, time %f\n", match_count, time);
  }
}
#endif

//------------------------------------------------------------------------------

#ifdef REGEX_BENCHMARK_SRELL

void benchmark_srell(const char* path) {
  std::string buf = read(path);

  const char* regex_email = "[\\w.+-]+@[\\w.-]+\\.[\\w.-]+";
  const char* regex_url   = "[\\w]+:\\/\\/[^\\/\\s?#]+[^\\s?#]+(?:\\?[^\\s#]*)?(?:#[^\\s]*)?";
  const char* regex_ip4   = "(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9])\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9])";

  {
    static srell::regex r(regex_email);
    srell::cregex_iterator it (buf.data(), buf.data() + buf.size(), r);
    srell::cregex_iterator end;

    int match_count = 0;
    double time = -timestamp_ms();
    while (it != end) {
      match_count++;
      it++;
    }
    time += timestamp_ms();
    printf("Email: Match count %4d, time %f\n", match_count, time);
  }

  {
    static srell::regex r(regex_url);
    srell::cregex_iterator it (buf.data(), buf.data() + buf.size(), r);
    srell::cregex_iterator end;

    int match_count = 0;
    double time = -timestamp_ms();
    while (it != end) {
      match_count++;
      it++;
    }
    time += timestamp_ms();
    printf("URL:   Match count %4d, time %f\n", match_count, time);
  }

  {
    static srell::regex r(regex_ip4);
    srell::cregex_iterator it (buf.data(), buf.data() + buf.size(), r);
    srell::cregex_iterator end;

    int match_count = 0;
    double time = -timestamp_ms();
    while (it != end) {
      match_count++;
      it++;
    }
    time += timestamp_ms();
    printf("IP4:   Match count %4d, time %f\n", match_count, time);
  }
}
#endif

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  printf("Regex benchmark shootout\n");

  const char* path = nullptr;
  if (argc > 1) path = argv[1];
  else          path = "../regex-benchmark/input-text.txt";

#ifdef REGEX_BENCHMARK_BASELINE
  printf("Benchmarking baseline:\n");
  benchmark_baseline(path);
  printf("\n");
#endif

#ifdef REGEX_BENCHMARK_MATCHERONI
  printf("Benchmarking Matcheroni:\n");
  benchmark_matcheroni(path);
  printf("\n");
#endif

#ifdef REGEX_BENCHMARK_STD_REGEX
  printf("Benchmarking std::regex:\n");
  benchmark_std_regex(path);
  printf("\n");
#endif

#ifdef REGEX_BENCHMARK_CTRE
  printf("Benchmarking CTRE:\n");
  benchmark_ctre(path);
  printf("\n");
#endif

#ifdef REGEX_BENCHMARK_BOOST
  printf("Benchmarking Boost:\n");
  benchmark_boost_regex(path);
  printf("\n");
#endif

#ifdef REGEX_BENCHMARK_SRELL
  printf("Benchmarking Srell:\n");
  benchmark_srell(path);
  printf("\n");
#endif

  return 0;
}

//------------------------------------------------------------------------------
