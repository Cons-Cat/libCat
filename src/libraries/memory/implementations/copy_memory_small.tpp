// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/simd>

namespace cat::detail {

// Power-of-2 branch tree for sizes `<= 127`. Head+tail overlap copies tolerate
// memmove-style overlap, and are valid for disjoint `memcpy` too.
[[clang::no_builtin("memcpy")]]
inline void
copy_memory_small(
   byte const* _Nonnull __restrict p_source,
   byte* _Nonnull __restrict p_destination, idx bytes
) {
   char* p_dest = reinterpret_cast<char*>(p_destination);
   char const* p_src = reinterpret_cast<char const*>(p_source);
   idx const byte_count = bytes;

   if (byte_count == 0u) {
      return;
   }

   if (byte_count < 4u) {
      idx const last_byte = (byte_count - 1u).to_idx().assert();
      idx const middle_byte = byte_count >> 1u;

      p_dest[0] = p_src[0];
      p_dest[last_byte] = p_src[last_byte];
      p_dest[middle_byte] = p_src[middle_byte];
      return;
   }

   if (byte_count < 8u) {
      uint4 head_word;
      uint4 tail_word;
      __builtin_memcpy_inline(&head_word, p_src, 4);
      __builtin_memcpy_inline(&tail_word, p_src + byte_count - 4, 4);
      __builtin_memcpy_inline(p_dest, &head_word, 4);
      __builtin_memcpy_inline(p_dest + byte_count - 4, &tail_word, 4);
      return;
   }

   if (byte_count < 16u) {
      uword head_word;
      uword tail_word;
      __builtin_memcpy_inline(&head_word, p_src, 8);
      __builtin_memcpy_inline(&tail_word, p_src + byte_count - 8, 8);
      __builtin_memcpy_inline(p_dest, &head_word, 8);
      __builtin_memcpy_inline(p_dest + byte_count - 8, &tail_word, 8);
      return;
   }

   if (byte_count < 32u) {
      char1x16 head_chunk;
      char1x16 tail_chunk;
      head_chunk.load_unaligned(p_src);
      tail_chunk.load_unaligned(p_src + byte_count - 16);
      head_chunk.store_unaligned(p_dest);
      tail_chunk.store_unaligned(p_dest + byte_count - 16);
      return;
   }

   if (byte_count < 64u) {
      char1x16 head_first;
      char1x16 head_second;
      char1x16 tail_first;
      char1x16 tail_second;
      head_first.load_unaligned(p_src);
      head_second.load_unaligned(p_src + 16);
      tail_first.load_unaligned(p_src + byte_count - 32);
      tail_second.load_unaligned(p_src + byte_count - 16);
      head_first.store_unaligned(p_dest);
      head_second.store_unaligned(p_dest + 16);
      tail_first.store_unaligned(p_dest + byte_count - 32);
      tail_second.store_unaligned(p_dest + byte_count - 16);
      return;
   }

   char1x16 head_chunks[4];
   char1x16 tail_chunks[4];
#pragma unroll
   for (idx vector_index = 0u; vector_index < 4u; ++vector_index) {
      head_chunks[vector_index].load_unaligned(p_src + (vector_index * 16u));
   }
#pragma unroll
   for (idx vector_index = 0u; vector_index < 4u; ++vector_index) {
      tail_chunks[vector_index].load_unaligned(
         p_src + byte_count - 64 + (vector_index * 16u)
      );
   }
#pragma unroll
   for (idx vector_index = 0u; vector_index < 4u; ++vector_index) {
      head_chunks[vector_index].store_unaligned(p_dest + (vector_index * 16u));
   }
#pragma unroll
   for (idx vector_index = 0u; vector_index < 4u; ++vector_index) {
      tail_chunks[vector_index].store_unaligned(
         p_dest + byte_count - 64 + (vector_index * 16u)
      );
   }
}

}  // namespace cat::detail
