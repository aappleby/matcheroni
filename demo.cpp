#include "Matcheroni.h"
#include "c_lexer.h"

#include <stdio.h>
#include <string>
#include <filesystem>
#include <vector>
#include <memory.h>
#include <chrono>
#include <set>

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
  int fail_count = 0;
};

const char* match_ifd_out(const char* text, void* ctx) {
  using prefix = Lit<"#if 0">;
  using suffix = Lit<"#endif">;
  using match = Seq<prefix, Any<Seq<Not<suffix>, Char<>>>, suffix>;

  return match::match(text, ctx);
}

const char* match_formfeed(const char* text, void* ctx) {
  if(uint8_t(text[0]) == 0x0C) return text + 1;
  return nullptr;
}

const char* match_cr(const char* text, void* ctx) {
  return Char<'\r'>::match(text, ctx);
}

/*
Conflict between multiline_comment, punct - multiline_comment wins
Conflict between preproc, punct           - preproc wins
Conflict between int, punct               - int wins
Conflict between oneline_comment, punct   - comment wins
Conflict between identifier, string       - string wins
Conflict between float, int               - float wins
Conflict between float, punct             - float wins
Conflict between byte order mark, utf8    - bom wins
Conflict between raw_string, identifier   - raw string wins

predefined constants and identifiers are gonna conflict - predef wins
*/

MatchTableEntry matcher_table[] = {
  { match_space,              0x804040, "space" },
  { match_newline,            0x404080, "newline" },
  { match_string_literal,     0x4488AA, "string" },            // must be before identifier
  { match_raw_string_literal, 0x88AAAA, "raw_string" },        // must be before identifier, should be 270 on gcc/gcc
  { match_identifier,         0xCCCC40, "identifier" },
  { match_multiline_comment,  0x66AA66, "multiline_comment" }, // must be before punct
  { match_oneline_comment,    0x008800, "oneline_comment" },   // must be before punct
  { match_preproc,            0xCC88CC, "preproc" },           // must be before punct
  { match_float,              0xFF88AA, "float" },             // must be before int
  { match_int,                0xFF8888, "int" },               // must be before punct
  { match_punct,              0x808080, "punct" },
  { match_character_constant, 0x44DDDD, "char_literal" },
  { match_splice,             0x00CCFF, "splice" },
  { match_formfeed,           0xFF00FF, "formfeed" },
  { match_byte_order_mark,    0xFF00FF, "byte order mark" },   // must be before utf8
};

//------------------------------------------------------------------------------

int total_files = 0;
int total_bytes = 0;
int source_files = 0;

std::vector<std::string> bad_files;
std::vector<std::string> skipped_files;

std::set<std::pair<int, int>> conflicts;
double lex_time = 0;

const char* current_filename = nullptr;

std::vector<std::string> invalid_files = {
  "-utf8.c",                          // Not dealing with utf8 yet
  "ucnid-7.c",                        // Intentionally invalid universal char code
  "/named-universal-char-escape",     // Named universal escapes not supported yet
  "/delimited-escape",                // Delimited escapes are C++23 only
  "/Wbidi",                           // We're not gonna support bidirectional characters quite yet
  "/dir-only",                        // The "directives only" tests intentionally have broken code
  "warn-normalized-1.c",              // Not lexable without preproc
  "warn-normalized-2.c",              // Not lexable without preproc
  "normalize-1.c",                    // Not lexable without preproc
  "normalize-2.c",                    // Not lexable without preproc
  "normalize-3.c",                    // Not lexable without preproc
  "normalize-4.c",                    // Not lexable without preproc
  "raw-string-10.c",                  // Not lexable without preproc
  "trigraphs.c",                      // We're missing some trigraph support? Removed in latest C draft...
  "warning-zero-in-literals-1.c",     // Nulls inside literals
  "raw-string-12.c",                  // Nulls inside raw strings
  "diagnostic-format-sarif-file-4.c", // Kanji identifier
  "lexstrng.c",                       // Trigraphs :P
  "multiline.c",                      // Not valid C
  "pr100646-2.c",                     // Backslash at end of file
  "mac-eol-at-eof.c",                 // Text with only \r and not \n?
};

//------------------------------------------------------------------------------

bool test_lex(const std::string& path, const std::string& text, bool echo) {
  current_filename = path.c_str();

  const char* cursor = text.data();
  const char* text_end = cursor + text.size() - 1;

  const int matcher_count = sizeof(matcher_table) / sizeof(matcher_table[0]);

  bool lex_ok = true;
  auto time_a = timestamp_ms();
  while(*cursor) {
    bool matched = false;

    for (int i = 0; i < matcher_count; i++) {
      auto& t = matcher_table[i];
      if (auto end = t.matcher_cb(cursor, nullptr)) {

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
          exit(1);
        }

        cursor = end;
        break;
      }
      else {
        t.fail_count++;
      }
    }

    if (!matched) {
      lex_ok = false;
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
      break;
    }
  }
  auto time_b = timestamp_ms();
  if (lex_ok) lex_time += time_b - time_a;

  if (echo) {
    set_color(0);
  }

  return lex_ok;
}

//------------------------------------------------------------------------------

bool test_lex(const std::string& path, size_t size, bool echo) {

  std::string text;
  text.resize(size + 1);
  memset(text.data(), 0, size + 1);
  FILE* f = fopen(path.c_str(), "rb");
  if (!f) {
    printf("Could not open %s!\n", path.c_str());
  }
  auto r = fread(text.data(), 1, size, f);
  total_bytes += size;
  text[size] = 0;
  fclose(f);

  if (text.find("dg-error") != std::string::npos ||
      text.find("dg-bogus") != std::string::npos ||
      text.find("@") != std::string::npos) { // gcc has some "stringizing literals with @ in them" test cases
    //printf("Skipping %s as it is intended to cause an error\n", path.c_str());
    skipped_files.push_back(path);
    return false;
  }

  return test_lex(path, text, echo);
}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
  printf("Matcheroni Demo\n");

  if (0) {
    //const char* text = R"('\'')";
    //printf("{%s}\n", text);
    //printf("{%s}\n", character_constant::match(text));

    //auto path = "testcases/test.c";
    //auto size = std::filesystem::file_size(path);
    //test_lex(path, size, true);
    return 0;
  }


  using rdit = std::filesystem::recursive_directory_iterator;
  const char* base_path = argc > 1 ? argv[1] : ".";

  bool echo = false;

  printf("Lexing source files in %s\n", base_path);
  auto time_a = timestamp_ms();
  for (const auto& f : rdit(base_path)) {
    if (f.is_regular_file()) {
      total_files++;
      auto& path = f.path().native();
      auto size = f.file_size();

      bool is_source =
        path.ends_with(".c") ||
        path.ends_with(".cpp") ||
        path.ends_with(".h") ||
        path.ends_with(".hpp");
      if (!is_source) continue;

      bool skip = false;
      for (const auto& f : invalid_files) {
        if (path.find(f) != std::string::npos) {
          skipped_files.push_back(path);
          skip = true;
          break;
        }
      }
      if (skip) continue;

      source_files++;
      test_lex(path, size, echo);
    }
  }
  auto time_b = timestamp_ms();
  auto total_time = time_b - time_a;

  printf("\n");
  printf("Total time %f msec\n", total_time);
  printf("Lex time   %f msec\n", lex_time);
  printf("Total files %d\n", total_files);
  printf("Total source files %d\n", source_files);
  printf("Total bytes %d\n", total_bytes);
  printf("Files skipped %ld\n", skipped_files.size());
  printf("Files with lex errors %ld\n", bad_files.size());
  printf("\n");

  const int matcher_count = sizeof(matcher_table) / sizeof(matcher_table[0]);
  for (int i = 0; i < matcher_count; i++) {
    printf("%-20s match %8d fail %8d\n",
      matcher_table[i].matcher_name,
      matcher_table[i].match_count,
      matcher_table[i].fail_count
    );
  }

  /*
  for (auto& f : skipped_files) {
    if (f.find("dir-only") != std::string::npos) {
      printf("%s\n", f.c_str());
    }
  }
  */

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
