// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/bit>
#include <cat/simd>

#include "copy_memory_small.tpp"

namespace cat::detail {

// Cosmopolitan-style backward 32-byte SIMD loop for overlapping memmove when
// `p_destination > p_source`.
[[clang::no_builtin("memmove")]]
inline void
copy_memory_backward_medium(
   byte const* _Nonnull p_source, byte* _Nonnull p_destination, idx bytes
) {
   char* p_dest = reinterpret_cast<char*>(p_destination);
   char const* p_src = reinterpret_cast<char const*>(p_source);
   iword bytes_remaining = bytes;

   constexpr idx l3_cache_size = 2_umi;
   x64::avx_simd<char> chunk_first;
   x64::avx_simd<char> chunk_second;

   if (bytes_remaining <= l3_cache_size) {
      while (bytes_remaining >= 64u) {
         bytes_remaining -= 64u;
         chunk_first.load_unaligned(p_src + bytes_remaining);
         chunk_second.load_unaligned(p_src + bytes_remaining + 32);
         chunk_first.store_unaligned(p_dest + bytes_remaining);
         chunk_second.store_unaligned(p_dest + bytes_remaining + 32);
      }
   } else {
      while (bytes_remaining > 64u
             && (cat::uintptr<char>{p_dest + bytes_remaining} % 32u) != 0u) {
         --bytes_remaining;
         idx const byte_index = bytes_remaining.to_idx().assert();
         p_dest[byte_index] = p_src[byte_index];
      }
      while (bytes_remaining >= 64u) {
         bytes_remaining -= 64u;
         chunk_first.load_unaligned(p_src + bytes_remaining);
         chunk_second.load_unaligned(p_src + bytes_remaining + 32);
         chunk_first.store_non_temporal(p_dest + bytes_remaining);
         chunk_second.store_non_temporal(p_dest + bytes_remaining + 32);
      }
      x64::sfence();
   }

   if (bytes_remaining > 0u) {
      copy_memory_small(
         p_source, p_destination, bytes_remaining.to_idx().assert()
      );
   }
}

}  // namespace cat::detail
