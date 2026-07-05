// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/simd>

#include "copy_memory_small.tpp"

namespace cat::detail {

// 32-byte SIMD loop for copies that are too large for `copy_memory_small` but
// smaller than one `$simd_switch` step.
[[clang::no_builtin("memcpy")]]
inline void
copy_memory_medium(byte const* _Nonnull __restrict p_source,
                   byte* _Nonnull __restrict p_destination, idx bytes) {
   char* p_dest = reinterpret_cast<char*>(p_destination);
   char const* p_src = reinterpret_cast<char const*>(p_source);
   idx const byte_count = bytes;
   idx byte_offset = 0u;

   // Strict forward 128-byte loop over 32-byte chunks. Copying in
   // increasing order keeps this valid for memmove when
   // `p_destination < p_source`, and the unwritten remainder is handed to
   // `copy_memory_small`.
   x64::avx_simd<char> chunk_first;
   x64::avx_simd<char> chunk_second;
   x64::avx_simd<char> chunk_third;
   x64::avx_simd<char> chunk_fourth;
   while (byte_offset + 128u <= byte_count) {
      chunk_first.load_unaligned(p_src + byte_offset);
      chunk_second.load_unaligned(p_src + byte_offset + 32);
      chunk_third.load_unaligned(p_src + byte_offset + 64);
      chunk_fourth.load_unaligned(p_src + byte_offset + 96);
      chunk_first.store_unaligned(p_dest + byte_offset);
      chunk_second.store_unaligned(p_dest + byte_offset + 32);
      chunk_third.store_unaligned(p_dest + byte_offset + 64);
      chunk_fourth.store_unaligned(p_dest + byte_offset + 96);
      byte_offset += 128u;
   }
   if (byte_offset + 64u <= byte_count) {
      chunk_first.load_unaligned(p_src + byte_offset);
      chunk_second.load_unaligned(p_src + byte_offset + 32);
      chunk_first.store_unaligned(p_dest + byte_offset);
      chunk_second.store_unaligned(p_dest + byte_offset + 32);
      byte_offset += 64u;
   }

   idx const tail_bytes = (byte_count - byte_offset).to_idx().assert();
   if (tail_bytes > 0u) {
      copy_memory_small(p_source + byte_offset, p_destination + byte_offset,
                        tail_bytes);
   }
}

}  // namespace cat::detail
