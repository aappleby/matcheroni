// SPDX-FileCopyrightText:  2023 Austin Appleby <aappleby@gmail.com>
// SPDX-License-Identifier: MIT License

#pragma once
#include <string>
#include <typeinfo>    // for type_info
#include <assert.h>

//------------------------------------------------------------------------------

#ifdef DEBUG
#define DCHECK(A) assert(A)
#else
#define DCHECK(A)
#endif

#define CHECK(A) assert(A)

//------------------------------------------------------------------------------

void set_color(uint32_t c);
void print_escaped(const char* s, int len, unsigned int color);
void print_typeid_name(const char* name);
double timestamp_ms();
std::string read(const char* path);
void read(const char* path, std::string& text);

void read(const char* path, char*& text_out, int& size_out);

//------------------------------------------------------------------------------

template<typename NodeType>
inline void print_class_name() {
  print_typeid_name(typeid(NodeType).name());
}

//------------------------------------------------------------------------------
