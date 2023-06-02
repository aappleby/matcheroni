#include "Matcheroni.h"
#include "c_lexer.h"

#include <stdio.h>
#include <string>
#include <filesystem>
#include <vector>
#include <memory.h>
#include <chrono>
#include <set>
#include <map>
#include <type_traits>

using namespace matcheroni;

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

void set_color(uint32_t c) {
  if (c) {
    printf("\u001b[38;2;%d;%d;%dm", (c >> 0) & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF);
  }
  else {
    printf("\u001b[0m");
  }
}

//------------------------------------------------------------------------------

int total_files = 0;
int total_bytes = 0;
int source_files = 0;

std::vector<std::string> bad_files;
std::vector<std::string> skipped_files;

double lex_time = 0;

const char* current_filename = nullptr;

std::vector<std::string> negative_test_cases = {
  "/objc/",                           // Objective C
  "/objc.dg/",                        // Objective C
  "/objc-obj-c++-shared/",            // Objective C
  "/asm/", // all assembler

  "19990407-1.c",                     // requires preproc
  "c2x-digit-separators-2.c",         // dg-error
  "c2x-utf8char-3.c",                 // dg-error
  "charconst.c",                      // dg-error
  "delimited-escape-seq-3.c",         // Empty delimited escape
  "delimited-escape-seq-4.c",         // Intentionally broken
  "delimited-escape-seq-5.c",         // Intentionally broken
  "delimited-escape-seq-6.c",         // Unterminated escape sequence
  "delimited-escape-seq-7.c",         // Unterminated escape sequence
  "dir-only-1.c",                     // -fdirectives-only
  "dir-only-8.c",                     // -fdirectives-only
  "escape-1.c",                       // dg-error
  "escape-2.c",                       // bad escape sequence
  "lexstrng.c",                       // trigraphs
  "mac-eol-at-eof.c",                 // Text with only \r and not \n?
  "macroargs.c",                      // preproc
  "missing-header-fixit-1.c",         // preproc
  "missing-header-fixit-2.c",         // preproc
  "multiline-2.c",                    // dg-error
  "multiline.c",                      // Not valid C
  "named-universal-char-escape-3.c",  // Empty named escape
  "named-universal-char-escape-5.c",  // Intentionally broken
  "named-universal-char-escape-6.c",  // Empty named escape
  "named-universal-char-escape-7.c",  // Empty named escape
  "normalize-1.c",                    // Not lexable without preproc
  "normalize-2.c",                    // Not lexable without preproc
  "normalize-3.c",                    // Not lexable without preproc
  "normalize-4.c",                    // Not lexable without preproc
  "pr100646-2.c",                     // Backslash at end of file
  "pr25993.c",                        // preproc
  "quote.c",                          // unterminated strings
  "raw-string-10.c",                  // Not lexable without preproc
  "raw-string-12.c",                  // Nulls inside raw strings
  "raw-string-14.c",                  // trigraphs
  "strify2.c",                        // preproc
  "trigraphs.c",                      // trigraphs
  "ucnid-7.c",                        // Intentionally invalid universal char code
  "ucs.c",                            // bad universal char name
  "utf16-4.c",                        // dg-error
  "utf32-4.c",                        // dg-error
  "warn-normalized-1.c",              // Not lexable without preproc
  "warn-normalized-2.c",              // Not lexable without preproc
  "warning-zero-in-literals-1.c",     // Nulls inside literals
  "include2.c", // dg-error
  "directiv.c", // dg-error
  "strify3.c", // x@y?
  "ppc-asm.h", // @notoc?
  "has-include-1.c", // dg-error
  "has-include-next-1.c", // dg-error
  "pragma-message.c", // dg-error
  "macspace1.c", // dg-error
  "macspace2.c", // dg-error
  "literals-2.c", // dg-error
  "recurse-3.c", // recursive macros that don't compile
  "if-2.c", // dg-error
  "abitest.h", // assembler
  "dce110_timing_generator.c", // "@TODOSTEREO" in if'd out block
  "carl9170/phy.h", // invalid backslash in macro
  "linux/elfnote.h", // assembly
  "boot/ppc_asm.h", // assembly
  "openacc_lib.h", // ...fortran?
  "encoding-issues-", // embedded nuls
};

std::map<LexemeType, uint32_t> lextype_to_color = {
  {LEX_INVALID    , 0x0000FF},
  {LEX_SPACE      , 0x804040},
  {LEX_NEWLINE    , 0x404080},
  {LEX_STRING     , 0x4488AA},
  {LEX_IDENTIFIER , 0xCCCC40},
  {LEX_COMMENT    , 0x66AA66},
  {LEX_PREPROC    , 0xCC88CC},
  {LEX_FLOAT      , 0xFF88AA},
  {LEX_INT        , 0xFF8888},
  {LEX_PUNCT      , 0x808080},
  {LEX_CHAR       , 0x44DDDD},
  {LEX_SPLICE     , 0x00CCFF},
  {LEX_FORMFEED   , 0xFF00FF},
  {LEX_EOF        , 0xFF00FF},
};

//------------------------------------------------------------------------------

bool test_lex(const std::string& path, const std::string& text, bool echo) {
  current_filename = path.c_str();

  const char* text_a = text.data();
  const char* text_b = text_a + text.size() - 1;
  const char* cursor = text_a;

  bool lex_ok = true;

  //----------------------------------------

  auto time_a = timestamp_ms();
  while(cursor) {

    auto lexeme = next_lexeme(cursor, text_b);

    if (lexeme.is_eof()) {
      break;
    }
    else if (lexeme.is_valid()) {
      if (lexeme.span_b > text_b) {
        printf("#### READ OFF THE END DURING %d\n", lexeme.type);
        printf("File %s:\n", path.c_str());
        printf("Cursor {%.40s}\n", cursor);
        exit(1);
      }

      if (echo) {
        set_color(lextype_to_color[lexeme.type]);
        if (lexeme.type == LEX_NEWLINE) {
          printf("\\n");
          fwrite(cursor, 1, lexeme.span_b - cursor, stdout);
        }
        else if (lexeme.type == LEX_SPACE) {
          for (int i = 0; i < (lexeme.span_b - cursor); i++) putc('.', stdout);
        }
        else {
          fwrite(cursor, 1, lexeme.span_b - cursor, stdout);
        }
      }
      cursor = lexeme.span_b;
    }
    else {
      lex_ok = false;
      printf("\n");
      printf("File %s:\n", path.c_str());
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

  //----------------------------------------

  if (cursor != text_b) {
    printf("???\n");
  }

  if (lex_ok) lex_time += time_b - time_a;

  if (echo) {
    set_color(0);
  }

  return lex_ok;
}

//------------------------------------------------------------------------------

void test_lex(const std::string& path, size_t size, bool echo) {

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

  bool skip = false;
  // Filter out all the header files that are actually assembly
  if (text.find("__ASSEMBLY__") != std::string::npos) skip = true;
  if (text.find("__ASSEMBLER__") != std::string::npos) skip = true;
  if (text.find("@function") != std::string::npos) skip = true;
  if (text.find("@progbits") != std::string::npos) skip = true;
  if (text.find(".macro") != std::string::npos) skip = true;

  if (skip) {
    //printf("Skipping %s as it is intended to cause an error\n", path.c_str());
    skipped_files.push_back(path);
    return;
  }

  source_files++;
  test_lex(path, text, echo);
}

//------------------------------------------------------------------------------

extern int test_c99_peg();

int main(int argc, char** argv) {
  printf("Matcheroni Demo\n");

  //test_c99_peg();
  //exit(0);

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
      for (const auto& f : negative_test_cases) {
        if (path.find(f) != std::string::npos) {
          skipped_files.push_back(path);
          skip = true;
          break;
        }
      }
      if (skip) continue;

      test_lex(path, size, echo);
    }
  }
  auto time_b = timestamp_ms();
  auto total_time = time_b - time_a;

  printf("\n");
  printf("Total time %f msec\n", total_time);
  printf("Lex time %f msec\n", lex_time);
  printf("Total files %d\n", total_files);
  printf("Total source files %d\n", source_files);
  printf("Total bytes %d\n", total_bytes);
  printf("Files skipped %ld\n", skipped_files.size());
  printf("Files with lex errors %ld\n", bad_files.size());
  printf("\n");

  dump_lexer_stats();

  return 0;
}
