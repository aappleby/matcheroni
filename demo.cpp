#include "Matcheroni.h"
#include "examples.h"

#include <stdio.h>
#include <string>
#include <filesystem>
#include <vector>
#include <memory.h>
#include <chrono>

using namespace matcheroni;

//\u0020;

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

int surely$thisdoesntwork;
int $butdoesthiswork;

void set_color(uint32_t c) {
  if (c) {
    printf("\u001b[38;2;%d;%d;%dm", (c >> 0) & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF);
  }
  else {
    printf("\u001b[0m");
  }
}


struct MatchTableEntry {
  matcher matcher_cb = nullptr;
  uint32_t color = 0;
  const char* matcher_name = nullptr;
  int match_count = 0;
};

const char* match_ifd_out(const char* text) {
  using prefix = Lit<"#if 0">;
  using suffix = Lit<"#endif">;
  using match = Seq<prefix, Any<Seq<Not<suffix>, Char<>>>, suffix>;

  return match::match(text);
}

const char* match_bom(const char* text) {
  if (!text) return nullptr;

  if (uint8_t(text[0]) == 0xFE &&
      uint8_t(text[1]) == 0xFF) {
    return text + 2;
  }

  if (uint8_t(text[0]) == 0xEF &&
      uint8_t(text[1]) == 0xBB &&
      uint8_t(text[2]) == 0xBF) {
    return text + 3;
  }

  return nullptr;
}

const char* match_formfeed(const char* text) {
  if(uint8_t(text[0]) == 0x0C) return text + 1;
  return nullptr;
}

MatchTableEntry matcher_table[] = {
  { match_bom,               0xFF00FF, "byte order mark" },
  { match_formfeed,          0xFF00FF, "formfeed" },
  { match_ifd_out,           0x202020, "#if 0" },
  { match_splice,            0x00CCFF, "splice" },
  { match_preproc,           0xCC88CC, "preproc" },
  { match_raw_string,        0x88AAAA, "raw_string" },
  { match_float,             0xFF88AA, "float" },
  { match_space,             0x804040, "space" },
  { match_newline,           0x404080, "newline" },
  //{ match_include_path,      0x88AACC, "include_path" },
  { match_string,            0x4488AA, "string" },
  { match_char_literal,      0x44DDDD, "char_literal" },
  { match_oneline_comment,   0x008800, "oneline_comment" },
  { match_multiline_comment, 0x66AA66, "multiline_comment" },
  { match_identifier,        0xCCCC40, "identifier" },
  { match_int,               0xFF8888, "int" },
  { match_punct,             0x808080, "punct" },
  { match_utf8,              0x000000, "utf8" },
};

//------------------------------------------------------------------------------

std::vector<std::string> bad_files;
std::vector<std::string> skipped_files;

void test_scan(const std::string path, size_t size, bool echo) {

  std::string text;
  text.resize(size + 1);
  memset(text.data(), 0, size + 1);
  FILE* f = fopen(path.c_str(), "rb");
  if (!f) {
    printf("Could not open %s!\n", path.c_str());
  }
  auto r = fread(text.data(), 1, size, f);
  text[size] = 0;
  fclose(f);

  // We're not gonna support UCNs in IDs quite yet
  if (path.find("/ucnid") != std::string::npos) {
    skipped_files.push_back(path);
    return;
  }

  // Same thing
  if (path.find("/named-universal-char-escape") != std::string::npos) {
    skipped_files.push_back(path);
    return;
  }

  if (path.find("/delimited-escape") != std::string::npos) {
    skipped_files.push_back(path);
    return;
  }

  // We're not gonna support bidirectional characters quite yet
  if (path.find("/Wbidi") != std::string::npos) {
    skipped_files.push_back(path);
    return;
  }

  std::vector<std::string> invalid_files = {
    "warn-normalized-1.c",  // Not lexable without preproc
    "warn-normalized-2.c",  // Not lexable without preproc
    "normalize-1.c",        // Not lexable without preproc
    "normalize-2.c",        // Not lexable without preproc
    "normalize-3.c",        // Not lexable without preproc
    "normalize-4.c",        // Not lexable without preproc
    "trigraphs.c",    // We're missing some trigraph support?
    "warning-zero-in-literals-1.c", // Nulls inside literals
    "raw-string-12.c", // Nulls inside raw strings
  };

  for (const auto& f : invalid_files) {
    if (path.find(f) != std::string::npos) {
      skipped_files.push_back(path);
      return;
    }
  }

  ///home/aappleby/projects/gcc/gcc/testsuite/gcc.dg/cpp/ucnid-10.c

  if (text.find("dg-error") != std::string::npos ||
      text.find("dg-bogus") != std::string::npos ||
      text.find("@") != std::string::npos) // gcc has some "stringizing literals with @ in them" test cases
      {
    //printf("Skipping %s as it is intended to cause an error\n", path.c_str());
    skipped_files.push_back(path);
    return;
  }

  const char* cursor = text.data();
  const char* text_end = cursor + size;

  const int matcher_count = sizeof(matcher_table) / sizeof(matcher_table[0]);

  while(*cursor) {
    bool matched = false;
    for (int i = 0; i < matcher_count; i++) {
      auto& t = matcher_table[i];
      if (auto end = t.matcher_cb(cursor)) {

        if (echo) {
          set_color(t.color);
          if (t.matcher_cb == match_newline) {
            printf("\\n");
            fwrite(cursor, 1, end - cursor, stdout);
          }
          else if (t.matcher_cb == match_space) {
            for (int i = 0; i < (end - cursor); i++) putc('.', stdout);
          }
          else {
            fwrite(cursor, 1, end - cursor, stdout);
          }
        }

        t.match_count++;
        matched = true;

        if (end > text_end) {
          printf("#### READ OFF THE END DURING %s\n", t.matcher_name);
          printf("File %s:\n", path.c_str());
          printf("Cursor {%.40s}\n", cursor);

          for (const char* c = cursor; c <= text_end; c++) {
            printf("%02x ", uint8_t(*c));
          }
          printf("\n");

          printf("%p\n", cursor);
          printf("%p\n", text_end);
          printf("%p\n", end);

          exit(1);
        }

        cursor = end;
        break;
      }
    }

    if (!matched) {
      printf("\n");
      printf("File %s:\n", path.c_str());
      //printf("Could not match {%.40s}\n", cursor);
      printf("Could not match\n");

      for (int i = 0; i < 40; i++) {
        if (!cursor[i]) break;
        if (cursor[i] == '\r') printf("{\\r}");
        else if (cursor[i] == '\n') printf("{\\n}");
        else if (cursor[i] == '\t') printf("{\\t}");
        else putc(cursor[i], stdout);
      }
      printf("\n");

      for (int i = 0; i < 40; i++) {
        if (!cursor[i]) break;
        printf("%02x ", uint8_t(cursor[i]));
      }
      printf("\n");
      bad_files.push_back(path);
      return;
    }
  }
}

//------------------------------------------------------------------------------

// Raw strings in a preprocessor macro break right now
/*
#pragma omp parallel num_threads(sizeof R"(
abc
)")
*/

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  printf("Matcheroni Demo\n");
  printf("\n");

  /*
  {
    const char* text = R"qqq(R"/^&|~!=,"'(a)/^&|~!=,"'"suffix)qqq";
    const char* end = match_raw_string2(text);

    printf("text {%s}\n", text);
    printf("end  {%s}\n", end);
    return 0;
  }
  */


  using rdit = std::filesystem::recursive_directory_iterator;
  const char* base_path = argc > 1 ? argv[1] : ".";

  int total_files = 0;
  int total_bytes = 0;
  int source_files = 0;

  bool echo = false;

  printf("Lexing source files\n");
  auto time_a = timestamp_ms();
  for (const auto& f : rdit(base_path)) {
    if (f.is_regular_file()) {
      total_files++;
      auto& path = f.path().native();
      auto size = f.file_size();

      //if (path != "/home/aappleby/projects/gcc/gcc/testsuite/c-c++-common/raw-string-2.c") continue;

      if (path.find("objc") != std::string::npos) continue;

      if (path.ends_with(".c") ||
          path.ends_with(".cpp") ||
          path.ends_with(".h") ||
          path.ends_with(".hpp")) {
        //printf("path %s\n", path.c_str());
        //printf(".");
        source_files++;
        test_scan(path, size, echo);
        total_bytes += size;
        set_color(0);
      }
    }
  }
  auto time_b = timestamp_ms();
  printf("\n");
  printf("Lexing took %f msec\n", time_b - time_a);
  printf("Total files %d\n", total_files);
  printf("Total source files %d\n", source_files);
  printf("Total bytes %d\n", total_bytes);
  printf("Files skipped %ld\n", skipped_files.size());
  printf("Files with lex errors %ld\n", bad_files.size());
  printf("\n");

  const int matcher_count = sizeof(matcher_table) / sizeof(matcher_table[0]);
  for (int i = 0; i < matcher_count; i++) {
    printf("%-20s %d\n", matcher_table[i].matcher_name, matcher_table[i].match_count);
  }

  /*
  if (bad_files.size()) {
    printf("\nFiles with lex errors:\n");
    for (const auto& p : bad_files) {
      printf("%s\n", p.c_str());
    }
  }
  */

  return 0;
}

//------------------------------------------------------------------------------
