#include "matcheroni/Matcheroni.hpp"
#include "matcheroni/Parseroni.hpp"

#include <stdint.h>
#include <vector>

using namespace matcheroni;

// 89 50 4E 47 0D 0A 1A 0A

using bspan = matcheroni::Span<uint8_t>;

struct ByteNode : public NodeBase<bspan> {
};

struct ByteContext : public NodeContext<bspan, ByteNode> {

  std::vector<uint32_t> stack;

  static bool atom_eq(uint8_t a, uint8_t b) { return a == b; }
  static bool atom_lt(uint8_t a, uint8_t b) { return a < b; }
  static bool atom_gt(uint8_t a, uint8_t b) { return a > b; }

  void rewind(bspan s) {
  }
};

template<auto B, auto... rest>
struct Bytes {
  static bspan match(ByteContext& ctx, bspan s) {
    matcheroni_assert(s.is_valid());
    if (s.is_empty()) return s.fail();
    if (s.a[0] != B) return s.fail();
    return Bytes<rest...>::match(ctx, s.advance(1));
  }
};

template<auto B>
struct Bytes<B> {
  static bspan match(ByteContext& ctx, bspan s) {
    matcheroni_assert(s.is_valid());
    if (s.is_empty()) return s.fail();
    if (s.a[0] != B) return s.fail();
    return s.advance(1);
  }
};

struct U8 {
  static bspan match(ByteContext& ctx, bspan s) {
    matcheroni_assert(s.is_valid());
    return s.len() < 4 ? s.fail() : s.advance(4);
  }
};

struct U16 {
  static bspan match(ByteContext& ctx, bspan s) {
    matcheroni_assert(s.is_valid());
    return s.len() < 4 ? s.fail() : s.advance(4);
  }
};

struct U32 {
  static bspan match(ByteContext& ctx, bspan s) {
    matcheroni_assert(s.is_valid());
    return s.len() < 4 ? s.fail() : s.advance(4);
  }
};

struct LenU32 {
  static bspan match(ByteContext& ctx, bspan s) {
    matcheroni_assert(s.is_valid());
    if (s.len() < 4) return s.fail();
    ctx.stack.push_back(*(uint32_t*)s.a);
    return s.advance(4);
  }
};

struct CRC32 {
  static bspan match(ByteContext& ctx, bspan s) {
    return s.fail();
  }
};

struct Blob {
  static bspan match(ByteContext& ctx, bspan s) {
    matcheroni_assert(s.is_valid());
    uint32_t len = ctx.stack.back();
    ctx.stack.pop_back();
    if (s.len() < len) return s.fail();
    return s.advance(len);
  }
};


void test_bytes() {
  ByteContext b;

  using signature = Bytes<0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A>;

  using chunk =
  Seq<
    Capture<"length", LenU32, ByteNode>,
    Capture<"chunk_type", U32, ByteNode>,
    Capture<"chunk_data", Blob, ByteNode>,
    Capture<"crc", CRC32, ByteNode>
  >;

  using IHDR =
  Seq<
    Capture<"length", LenU32, ByteNode>,
    Capture<"chunk_type", Lit<"IHDR">, ByteNode>,
    Capture<"chunk_data", Blob, ByteNode>,
    Capture<"crc", CRC32, ByteNode>
  >;

  using pngfile =
  Seq<
    Capture<"signature", signature, ByteNode>,
    Any<
      Capture<"chunk", Oneof<chunk, IHDR>, ByteNode>
    >
  >;

  uint8_t bytes[] = { 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04 };
  bspan data(bytes, bytes + sizeof(bytes));

  bspan tail = pngfile::match(b, data);

}
