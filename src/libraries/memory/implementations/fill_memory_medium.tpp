// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/bit>
#include <cat/simd>

#include "fill_memory_small.tpp"

namespace cat::detail {

// Forward 32-byte SIMD fill for sizes above `fill_memory_small`.
[[clang::no_builtin("memset")]]
inline void
fill_memory_medium(byte* _Nonnull p_destination, byte byte_value, idx bytes) {
   char* p_dest = reinterpret_cast<char*>(p_destination);
   idx const byte_count = bytes;

   if (byte_count < 64u) {
      fill_memory_small(p_destination, byte_value, bytes);
      return;
   }

   char1x32 chunk;
   chunk.fill(byte_value);

   constexpr idx l3_cache_size = 2_umi;
   idx byte_offset = 0u;

   if (byte_count <= l3_cache_size) {
      while (byte_offset + 128u < byte_count) {
         chunk.store_unaligned(p_dest + byte_offset);
         chunk.store_unaligned(p_dest + byte_offset + 32);
         chunk.store_unaligned(p_dest + byte_offset + 64);
         chunk.store_unaligned(p_dest + byte_offset + 96);
         byte_offset += 128u;
      }

      while (byte_offset + 64u < byte_count) {
         chunk.store_unaligned(p_dest + byte_offset);
         chunk.store_unaligned(p_dest + byte_offset + 32);
         byte_offset += 64u;
      }
   } else {
      while (byte_offset + 64u < byte_count
             && (cat::uintptr<char>{p_dest + byte_offset} % 32u) != 0u) {
         p_destination[byte_offset++] = byte_value;
      }

      while (byte_offset + 64u < byte_count) {
         chunk.store_non_temporal(p_dest + byte_offset);
         chunk.store_non_temporal(p_dest + byte_offset + 32);
         byte_offset += 64u;
      }
      x64::sfence();
   }

   // Final overlapping 64-byte block. Re-storing already-filled bytes is
   // harmless because the fill value is constant, and it removes the tail
   // branch tree.
   chunk.store_unaligned(p_dest + byte_count - 64);
   chunk.store_unaligned(p_dest + byte_count - 32);
}

}  // namespace cat::detail
