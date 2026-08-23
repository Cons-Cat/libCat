// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/simd>

namespace cat::detail {

[[gnu::always_inline]]
inline void
copy_memory_small_1_to_3(
   byte const* _Nonnull __restrict p_source,
   byte* _Nonnull __restrict p_destination, idx bytes
) {
   char* p_dest = reinterpret_cast<char*>(p_destination);
   char const* p_src = reinterpret_cast<char const*>(p_source);
   idx last_byte;
   last_byte.raw = bytes.raw - 1u;
   idx const middle_byte = bytes >> 1u;
   p_dest[0] = p_src[0];
   p_dest[last_byte] = p_src[last_byte];
   p_dest[middle_byte] = p_src[middle_byte];
}

[[gnu::always_inline]]
inline void
copy_memory_small_4_to_7(
   byte const* _Nonnull __restrict p_source,
   byte* _Nonnull __restrict p_destination, idx bytes
) {
   uint4 head_word;
   uint4 tail_word;
   __builtin_memcpy_inline(&head_word, p_source, 4);
   __builtin_memcpy_inline(&tail_word, p_source + bytes - 4, 4);
   __builtin_memcpy_inline(p_destination, &head_word, 4);
   __builtin_memcpy_inline(p_destination + bytes - 4, &tail_word, 4);
}

[[gnu::always_inline]]
inline void
copy_memory_small_8_to_15(
   byte const* _Nonnull __restrict p_source,
   byte* _Nonnull __restrict p_destination, idx bytes
) {
   uword head_word;
   uword tail_word;
   __builtin_memcpy_inline(&head_word, p_source, 8);
   __builtin_memcpy_inline(&tail_word, p_source + bytes - 8, 8);
   __builtin_memcpy_inline(p_destination, &head_word, 8);
   __builtin_memcpy_inline(p_destination + bytes - 8, &tail_word, 8);
}

[[gnu::always_inline]]
inline void
copy_memory_small_16_to_31(
   byte const* _Nonnull __restrict p_source,
   byte* _Nonnull __restrict p_destination, idx bytes
) {
   char const* p_src = reinterpret_cast<char const*>(p_source);
   char* p_dest = reinterpret_cast<char*>(p_destination);
   char1x16 head_chunk;
   char1x16 tail_chunk;
   head_chunk.load(p_src);
   tail_chunk.load(p_src + bytes - 16);
   head_chunk.store(p_dest);
   tail_chunk.store(p_dest + bytes - 16);
}

[[gnu::always_inline]]
inline void
copy_memory_small_32_to_63(
   byte const* _Nonnull __restrict p_source,
   byte* _Nonnull __restrict p_destination, idx bytes
) {
   char const* p_src = reinterpret_cast<char const*>(p_source);
   char* p_dest = reinterpret_cast<char*>(p_destination);
   char1x16 head_first;
   char1x16 head_second;
   char1x16 tail_first;
   char1x16 tail_second;
   head_first.load(p_src);
   head_second.load(p_src + 16);
   tail_first.load(p_src + bytes - 32);
   tail_second.load(p_src + bytes - 16);
   head_first.store(p_dest);
   head_second.store(p_dest + 16);
   tail_first.store(p_dest + bytes - 32);
   tail_second.store(p_dest + bytes - 16);
}

[[gnu::always_inline]]
inline void
copy_memory_small_64_to_127(
   byte const* _Nonnull __restrict p_source,
   byte* _Nonnull __restrict p_destination, idx bytes
) {
   char const* p_src = reinterpret_cast<char const*>(p_source);
   char* p_dest = reinterpret_cast<char*>(p_destination);
   char1x16 head_chunks[4];
   char1x16 tail_chunks[4];
#pragma unroll
   for (idx vector_index = 0u; vector_index < 4u; ++vector_index) {
      head_chunks[vector_index].load(p_src + (vector_index * 16u));
   }
#pragma unroll
   for (idx vector_index = 0u; vector_index < 4u; ++vector_index) {
      tail_chunks[vector_index].load(
         p_src + bytes - 64 + (vector_index * 16u)
      );
   }
#pragma unroll
   for (idx vector_index = 0u; vector_index < 4u; ++vector_index) {
      head_chunks[vector_index].store(p_dest + (vector_index * 16u));
   }
#pragma unroll
   for (idx vector_index = 0u; vector_index < 4u; ++vector_index) {
      tail_chunks[vector_index].store(
         p_dest + bytes - 64 + (vector_index * 16u)
      );
   }
}

// Power-of-2 branch tree for sizes `<= 127`.
[[clang::no_builtin("memcpy")]]
inline void
copy_memory_small(
   byte const* _Nonnull __restrict p_source,
   byte* _Nonnull __restrict p_destination, idx bytes
) {
   if (bytes == 0u) {
      return;
   }
   if (bytes < 4u) {
      copy_memory_small_1_to_3(p_source, p_destination, bytes);
      return;
   }
   if (bytes < 8u) {
      copy_memory_small_4_to_7(p_source, p_destination, bytes);
      return;
   }
   if (bytes < 16u) {
      copy_memory_small_8_to_15(p_source, p_destination, bytes);
      return;
   }
   if (bytes < 32u) {
      copy_memory_small_16_to_31(p_source, p_destination, bytes);
      return;
   }
   if (bytes < 64u) {
      copy_memory_small_32_to_63(p_source, p_destination, bytes);
      return;
   }
   copy_memory_small_64_to_127(p_source, p_destination, bytes);
}

}  // namespace cat::detail
