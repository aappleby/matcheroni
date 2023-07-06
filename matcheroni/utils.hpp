#pragma once
#include <assert.h>
#include <chrono>
#include <filesystem>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>    // for exit
#include <string>
#include <typeinfo>    // for type_info

//------------------------------------------------------------------------------

inline void set_color(uint32_t c) {
  if (c) {
    printf("\u001b[38;2;%d;%d;%dm", (c >> 0) & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF);
  }
  else {
    printf("\u001b[0m");
  }
}

//------------------------------------------------------------------------------

inline void print_escaped(const char* s, int len, unsigned int color) {
  if (len < 0) {
    exit(1);
  }
  set_color(color);
  while(len) {
    auto c = *s++;
    if      (c == 0)    break;
    else if (c == ' ')  putc(' ', stdout);
    else if (c == '\n') putc(' ', stdout);
    else if (c == '\r') putc(' ', stdout);
    else if (c == '\t') putc(' ', stdout);
    else                putc(c,   stdout);
    len--;
  }
  while(len--) putc('#', stdout);
  set_color(0);

  return;
}

//------------------------------------------------------------------------------

inline void print_typeid_name(const char* name) {
  int name_len = 0;
  if (sscanf(name, "%d", &name_len)) {
    while((*name >= '0') && (*name <= '9')) name++;
    for (int i = 0; i < name_len; i++) {
      putc(name[i], stdout);
    }
  }
  else {
    printf("%s", name);
  }
}

//------------------------------------------------------------------------------

template<typename NodeType>
inline void print_class_name() {
  const char* name = typeid(NodeType).name();
  int name_len = 0;
  if (sscanf(name, "%d", &name_len)) {
    while((*name >= '0') && (*name <= '9')) name++;
  }

  for (int i = 0; i < name_len; i++) {
    putc(name[i], stdout);
  }
}

//------------------------------------------------------------------------------

#ifdef DEBUG
#define DCHECK(A) assert(A)
#else
#define DCHECK(A)
#endif

#define CHECK(A) assert(A)

//------------------------------------------------------------------------------

inline double timestamp_ms() {
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

inline std::string read(const char* path) {
  auto size = std::filesystem::file_size(path);
  std::string buf;
  buf.resize(size);
  FILE* f = fopen(path, "rb");
  auto _ = fread(buf.data(), size, 1, f);
  fclose(f);
  return buf;
}

//------------------------------------------------------------------------------
