#include "c_parser.h"
#include "c_lexer.h"

#include <filesystem>
#include <array>

//------------------------------------------------------------------------------

// MUST BE SORTED CASE-SENSITIVE
std::array builtin_type_base = {
  "FILE", // used in fprintf.c torture test
  "_Bool",
  "_Complex", // yes this is both a prefix and a type :P
  "__INT16_TYPE__",
  "__INT32_TYPE__",
  "__INT64_TYPE__",
  "__INT8_TYPE__",
  "__INTMAX_TYPE__",
  "__INTPTR_TYPE__",
  "__INT_FAST16_TYPE__",
  "__INT_FAST32_TYPE__",
  "__INT_FAST64_TYPE__",
  "__INT_FAST8_TYPE__",
  "__INT_LEAST16_TYPE__",
  "__INT_LEAST32_TYPE__",
  "__INT_LEAST64_TYPE__",
  "__INT_LEAST8_TYPE__",
  "__PTRDIFF_TYPE__",
  "__SIG_ATOMIC_TYPE__",
  "__SIZE_TYPE__",
  "__UINT16_TYPE__",
  "__UINT32_TYPE__",
  "__UINT64_TYPE__",
  "__UINT8_TYPE__",
  "__UINTMAX_TYPE__",
  "__UINTPTR_TYPE__",
  "__UINT_FAST16_TYPE__",
  "__UINT_FAST32_TYPE__",
  "__UINT_FAST64_TYPE__",
  "__UINT_FAST8_TYPE__",
  "__UINT_LEAST16_TYPE__",
  "__UINT_LEAST32_TYPE__",
  "__UINT_LEAST64_TYPE__",
  "__UINT_LEAST8_TYPE__",
  "__WCHAR_TYPE__",
  "__WINT_TYPE__",
  "__builtin_va_list",
  "__imag__",
  "__int128",
  "__real__",
  "bool",
  "char",
  "double",
  "float",
  "int",
  "int16_t",
  "int32_t",
  "int64_t",
  "int8_t",
  "long",
  "short",
  "signed",
  "size_t", // used in fputs-lib.c torture test
  "uint16_t",
  "uint32_t",
  "uint64_t",
  "uint8_t",
  "unsigned",
  "va_list", // technically part of the c library, but it shows up in stdarg test files
  "void",
  "wchar_t",
};

template<typename T, ptrdiff_t N>
constexpr ptrdiff_t arraysize(const T(&)[N]) { return N; }

template<typename T, ptrdiff_t N>
constexpr inline int topbit2(const T(&)[N]) {
  ptrdiff_t x = N;
  ptrdiff_t bit = 1;
  ptrdiff_t top = 0;
  while(x) {
    if (x & bit) {
      top = bit;
      x &= ~bit;
    }
    bit <<= 1;
  }
  return top;
}











constexpr int builtin_type_base_count  = sizeof(builtin_type_base)/sizeof(builtin_type_base[0]);
constexpr int builtin_type_base_topbit = topbit(builtin_type_base_count);

// MUST BE SORTED CASE-SENSITIVE
const char* builtin_type_prefix[] = {
  "_Complex",
  "__complex__",
  "__imag__",
  "__real__",
  "__signed__",
  "__unsigned__",
  "long",
  "short",
  "signed",
  "unsigned",
};

constexpr int builtin_type_prefix_count  = sizeof(builtin_type_prefix) / sizeof(builtin_type_prefix[0]);
constexpr int builtin_type_prefix_topbit = topbit(builtin_type_prefix_count);

// MUST BE SORTED CASE-SENSITIVE
const char* builtin_type_suffix[] = {
  // Why, GCC, why?
  "_Complex",
  "__complex__",
};

constexpr int builtin_type_suffix_count  = sizeof(builtin_type_suffix) / sizeof(builtin_type_suffix[0]);
constexpr int builtin_type_suffix_topbit = topbit(builtin_type_suffix_count);

//------------------------------------------------------------------------------

double timestamp_ms() {
  //printf("%d\n", blep.s);
  //printf("%d\n", blep.t);

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

template<typename P>
struct ProgressBar {
  static Token* match(void* ctx, Token* a, Token* b) {
    printf("%.40s\n", a->lex->span_a);
    return P::match(ctx, a, b);
  }
};

struct NodeToplevelDeclaration {
  using pattern =
  Oneof<
    Atom<';'>,
    NodeStatementTypedef,
    Seq<NodeStruct,   Atom<';'>>,
    Seq<NodeUnion,    Atom<';'>>,
    Seq<NodeTemplate, Atom<';'>>,
    Seq<NodeClass,    Atom<';'>>,
    Seq<NodeEnum,     Atom<';'>>,
    NodeFunction,
    Seq<NodeDeclaration, Atom<';'>>
  >;
};

struct NodeTranslationUnit : public ParseNode {};

//------------------------------------------------------------------------------

C99Parser::C99Parser() {
  text.reserve(65536);
  lexemes.reserve(65536);
  tokens.reserve(65536);
}

//------------------------------------------------------------------------------

void C99Parser::reset() {
  cleanup_accum -= timestamp_ms();

  text.clear();
  lexemes.clear();
  tokens.clear();
  ParseNode::clear_slabs();

  class_types.clear();
  struct_types.clear();
  union_types.clear();
  enum_types.clear();
  typedef_types.clear();

  cleanup_accum += timestamp_ms();
}

//------------------------------------------------------------------------------

void C99Parser::load(const std::string& path) {
  io_accum -= timestamp_ms();
  auto size = std::filesystem::file_size(path);
  text.resize(size);
  memset(text.data(), 0, size);
  FILE* file = fopen(path.c_str(), "rb");
  if (!file) {
    printf("Could not open %s!\n", path.c_str());
  }
  auto r = fread(text.data(), 1, size, file);
  fclose(file);

  for (auto c : text) if (c == '\n') file_lines++;

  //if (text.find("#define") != std::string::npos) {
  //  return -1;
  //}

  io_accum += timestamp_ms();

  file_keep++;
  file_bytes += text.size();
}

//------------------------------------------------------------------------------

void C99Parser::lex() {
  lex_accum -= timestamp_ms();

  auto text_a = text.data();
  auto text_b = text_a + text.size();

  const char* cursor = text_a;
  while(cursor) {
    auto lex = next_lexeme(nullptr, cursor, text_b);
    lexemes.push_back(lex);
    if (lex.type == LEX_EOF) {
      break;
    }
    cursor = lex.span_b;
  }

  for (auto i = 0; i < lexemes.size(); i++) {
    auto l = &lexemes[i];
    if (!l->is_gap() && !l->is_preproc()) {
      tokens.push_back(Token(l));
    }
  }

  assert(tokens.back().lex->is_eof());

  lex_accum += timestamp_ms();

  tok_a = tokens.data();
  tok_b = tokens.data() + tokens.size() - 1;
}

//------------------------------------------------------------------------------

ParseNode* C99Parser::parse() {
  parse_accum -= timestamp_ms();

  //auto result = NodeTranslationUnit::match(this, tok_a, tok_b);

  NodeTranslationUnit* root = nullptr;

  auto cursor = tok_a;

  {
    using pattern = Any<
      Oneof<
        C99Parser::node_maker<NodePreproc>,
        NodeToplevelDeclaration::pattern
      >
    >;

    cursor = pattern::match(this, cursor, tok_b);
    if (cursor) {
      assert(cursor == tok_b);
      root = new NodeTranslationUnit();
      root->init(tok_a, tok_b);
    }
  }

  parse_accum += timestamp_ms();

  if (root) file_pass++;
  this->root = root;

  return root;
}

















const char* sst_lookup(const char* a, const char* b, const char* const* tab, int count, int topbit) {
  int bit = topbit;
  int index = 0;

  while(1) {
    auto new_index = index | bit;
    if (new_index < builtin_type_base_count) {
      auto lit = builtin_type_base[new_index];
      auto c = cmp_span_lit(a, b, lit);
      if (c == 0) return lit;
      if (c > 0) index = new_index;
    }
    if (bit == 0) return nullptr;
    bit >>= 1;
  }
}





//------------------------------------------------------------------------------

Token* C99Parser::match_builtin_type_base(Token* a, Token* b) {
  if (!a || a == b) return nullptr;

  auto result = sst_lookup(a->lex->span_a, a->lex->span_b,
    builtin_type_base.data(), builtin_type_base_count, builtin_type_base_topbit);

  return result ? a + 1 : nullptr;
}

Token* C99Parser::match_builtin_type_prefix(Token* a, Token* b) {
  if (!a || a == b) return nullptr;

  auto result = sst_lookup(a->lex->span_a, a->lex->span_b,
    builtin_type_prefix, builtin_type_prefix_count, builtin_type_prefix_topbit);

  return result ? a + 1 : nullptr;
}

Token* C99Parser::match_builtin_type_suffix(Token* a, Token* b) {
  if (!a || a == b) return nullptr;

  auto taa = a->lex->span_a;
  auto tab = a->lex->span_b;

  for (auto tb : builtin_type_suffix) {
    if (cmp_span_lit(taa, tab, tb) == 0) return a + 1;
  }

  return nullptr;
}

//------------------------------------------------------------------------------
// FIXME should not be using text() here to avoid a temporary string allocation

struct NodeClassType   : public ParseNode {};
struct NodeStructType  : public ParseNode {};
struct NodeUnionType   : public ParseNode {};
struct NodeEnumType    : public ParseNode {};
struct NodeTypedefType : public ParseNode {};

Token* C99Parser::match_class_type(Token* a, Token* b) {
  if (a && class_types.contains(a->lex->text())) {
    auto end = a + 1;
    a->top = new NodeClassType();
    a->top->init(a, end);
    return end;
  }
  return nullptr;
}

Token* C99Parser::match_struct_type(Token* a, Token* b) {
  if (a && struct_types.contains(a->lex->text())) {
    auto end = a + 1;
    a->top = new NodeStructType();
    a->top->init(a, end);
    return end;
  }
  return nullptr;
}

Token* C99Parser::match_union_type(Token* a, Token* b) {
  if (a && union_types.contains(a->lex->text())) {
    auto end = a + 1;
    a->top = new NodeUnionType();
    a->top->init(a, end);
    return end;
  }
  return nullptr;
}

Token* C99Parser::match_enum_type(Token* a, Token* b) {
  if (a && enum_types.contains(a->lex->text())) {
    auto end = a + 1;
    a->top = new NodeEnumType();
    a->top->init(a, end);
    return end;
  }
  return nullptr;
}

Token* C99Parser::match_typedef_type(Token* a, Token* b) {
  if (a && typedef_types.contains(a->lex->text())) {
    auto end = a + 1;
    a->top = new NodeTypedefType();
    a->top->init(a, end);
    return end;
  }
  return nullptr;
}

//------------------------------------------------------------------------------






























//------------------------------------------------------------------------------

void C99Parser::dump_stats() {
  double total_time = io_accum + lex_accum + parse_accum + cleanup_accum;

  if (file_keep == 10000 && ParseNode::constructor_count != 565766993) {
    set_color(0x008080FF);
    printf("############## NODE COUNT MISMATCH\n");
    set_color(0);
  }
  printf("Total time     %f msec\n", total_time);
  printf("IO time        %f msec\n", io_accum);
  printf("Lexing time    %f msec\n", lex_accum);
  printf("Parsing time   %f msec\n", parse_accum);
  printf("Cleanup time   %f msec\n", cleanup_accum);
  printf("\n");
  printf("Bytes/sec      %f\n", 1000.0 * double(file_bytes) / double(total_time));
  printf("Lines/sec      %f\n", 1000.0 * double(file_lines) / double(total_time));
  printf("\n");
  printf("Total nodes    %d\n", ParseNode::constructor_count);
  printf("Node pool      %ld bytes\n", ParseNode::max_size);
  printf("Files total    %d\n", file_total);
  printf("Files filtered %d\n", file_total - file_keep);
  printf("Files kept     %d\n", file_keep);
  printf("Files bytes    %d\n", file_bytes);
  printf("Files lines    %d\n", file_lines);
  printf("Files pass     %d\n", file_pass);
  printf("Files fail     %d\n", file_keep - file_pass);
  printf("Average line   %f bytes\n", double(file_bytes) / double(file_lines));

  if (file_keep != file_pass) {
    set_color(0x008080FF);
    printf("##################\n");
    printf("##     FAIL     ##\n");
    printf("##################\n");
    set_color(0);
  }
  else {
    set_color(0x0080FF80);
    printf("##################\n");
    printf("##     PASS     ##\n");
    printf("##################\n");
    set_color(0);
  }
}

//------------------------------------------------------------------------------

const char* lex_to_str(const Lexeme& lex) {
  switch(lex.type) {
    case LEX_INVALID:    return "LEX_INVALID";
    case LEX_SPACE:      return "LEX_SPACE";
    case LEX_NEWLINE:    return "LEX_NEWLINE";
    case LEX_STRING:     return "LEX_STRING";
    case LEX_IDENTIFIER: return "LEX_IDENTIFIER";
    case LEX_COMMENT:    return "LEX_COMMENT";
    case LEX_PREPROC:    return "LEX_PREPROC";
    case LEX_FLOAT:      return "LEX_FLOAT";
    case LEX_INT:        return "LEX_INT";
    case LEX_PUNCT:      return "LEX_PUNCT";
    case LEX_CHAR:       return "LEX_CHAR";
    case LEX_SPLICE:     return "LEX_SPLICE";
    case LEX_FORMFEED:   return "LEX_FORMFEED";
    case LEX_EOF:        return "LEX_EOF";
  }
  return "<invalid>";
}

uint32_t lex_to_color(const Lexeme& lex) {
  switch(lex.type) {
    case LEX_INVALID    : return 0x0000FF;
    case LEX_SPACE      : return 0x804040;
    case LEX_NEWLINE    : return 0x404080;
    case LEX_STRING     : return 0x4488AA;
    case LEX_IDENTIFIER : return 0xCCCC40;
    case LEX_COMMENT    : return 0x66AA66;
    case LEX_PREPROC    : return 0xCC88CC;
    case LEX_FLOAT      : return 0xFF88AA;
    case LEX_INT        : return 0xFF8888;
    case LEX_PUNCT      : return 0x808080;
    case LEX_CHAR       : return 0x44DDDD;
    case LEX_SPLICE     : return 0x00CCFF;
    case LEX_FORMFEED   : return 0xFF00FF;
    case LEX_EOF        : return 0xFF00FF;
  }
  return 0x0000FF;
}

std::string escape_span(ParseNode* n) {
  if (!n->tok_a || !n->tok_b) {
    return "<bad span>";
  }

  if (n->tok_a == n->tok_b) {
    return "<zero span>";
  }

  auto lex_a = n->tok_a->lex;
  auto lex_b = (n->tok_b - 1)->lex;
  auto len = lex_b->span_b - lex_a->span_a;

  std::string result;
  for (auto i = 0; i < len; i++) {
    auto c = lex_a->span_a[i];
    if (c == '\n') {
      result.push_back('\\');
      result.push_back('n');
    }
    else if (c == '\r') {
      result.push_back('\\');
      result.push_back('r');
    }
    else if (c == '\t') {
      result.push_back('\\');
      result.push_back('t');
    }
    else {
      result.push_back(c);
    }
    if (result.size() >= 80) break;
  }

  return result;
}


void dump_tree(ParseNode* n, int max_depth, int indentation) {
  if (max_depth && indentation == max_depth) return;

  for (int i = 0; i < indentation; i++) printf(" | ");

  if (n->tok_a) set_color(lex_to_color(*(n->tok_a->lex)));
  //if (!field.empty()) printf("%-10.10s : ", field.c_str());

  n->print_class_name();

  printf(" '%s'\n", escape_span(n).c_str());

  if (n->tok_a) set_color(0);

  for (auto c = n->head; c; c = c->next) {
    dump_tree(c, max_depth, indentation + 1);
  }
}

//------------------------------------------------------------------------------

void dump_lexeme(const Lexeme& l) {
  int len = l.span_b - l.span_a;
  for (int i = 0; i < len; i++) {
    auto c = l.span_a[i];
    if (c == '\n' || c == '\t' || c == '\r') {
      putc('@', stdout);
    }
    else {
      putc(l.span_a[i], stdout);
    }
  }
}

void C99Parser::dump_lexemes() {
  for(auto& l : lexemes) {
    dump_lexeme(l);
    printf("\n");
  }
}

//------------------------------------------------------------------------------

void C99Parser::dump_tokens() {
  for (auto& t : tokens) {
    // Dump token
    printf("tok %p - ", &t);
    if (t.lex->is_eof()) {
      printf("<eof>");
    }
    else {
      dump_lexeme(*t.lex);
    }
    printf("\n");

    // Dump top node
    printf("  ");
    if (t.top) {
      printf("top %p -> ", t.top);
      t.top->print_class_name();
      printf(" -> %p ", t.top->tok_b);
    }
    else {
      printf("top %p", t.top);
    }
    printf("\n");
  }
}

//------------------------------------------------------------------------------
