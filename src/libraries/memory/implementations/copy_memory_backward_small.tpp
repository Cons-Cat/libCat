// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/simd>

namespace cat::detail {

[[gnu::always_inline]]
inline void
copy_memory_backward_small_1_to_3(
   byte const* _Nonnull p_source, byte* _Nonnull p_destination, idx bytes
) {
   char* p_dest = reinterpret_cast<char*>(p_destination);
   char const* p_src = reinterpret_cast<char const*>(p_source);
   idx last_byte;
   last_byte.raw = bytes.raw - 1u;
   idx const middle_byte = bytes >> 1u;
   char const head = p_src[0];
   char const tail = p_src[last_byte];
   char const middle = p_src[middle_byte];
   p_dest[0] = head;
   p_dest[last_byte] = tail;
   p_dest[middle_byte] = middle;
}

[[gnu::always_inline]]
inline void
copy_memory_backward_small_4_to_7(
   byte const* _Nonnull p_source, byte* _Nonnull p_destination, idx bytes
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
copy_memory_backward_small_8_to_15(
   byte const* _Nonnull p_source, byte* _Nonnull p_destination, idx bytes
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
copy_memory_backward_small_16_to_32(
   byte const* _Nonnull p_source, byte* _Nonnull p_destination, idx bytes
) {
   char const* p_src = reinterpret_cast<char const*>(p_source);
   char* p_dest = reinterpret_cast<char*>(p_destination);
   char1x16 head_chunk;
   char1x16 tail_chunk;
   head_chunk.load_unaligned(p_src);
   tail_chunk.load_unaligned(p_src + bytes - 16);
   head_chunk.store_unaligned(p_dest);
   tail_chunk.store_unaligned(p_dest + bytes - 16);
}

[[gnu::always_inline]]
inline void
copy_memory_backward_small_33_to_64(
   byte const* _Nonnull p_source, byte* _Nonnull p_destination, idx bytes
) {
   char const* p_src = reinterpret_cast<char const*>(p_source);
   char* p_dest = reinterpret_cast<char*>(p_destination);
   char1x16 head_first;
   char1x16 head_second;
   char1x16 tail_first;
   char1x16 tail_second;
   head_first.load_unaligned(p_src);
   head_second.load_unaligned(p_src + 16);
   tail_first.load_unaligned(p_src + bytes - 32);
   tail_second.load_unaligned(p_src + bytes - 16);
   head_first.store_unaligned(p_dest);
   head_second.store_unaligned(p_dest + 16);
   tail_first.store_unaligned(p_dest + bytes - 32);
   tail_second.store_unaligned(p_dest + bytes - 16);
}

[[gnu::always_inline]]
inline void
copy_memory_backward_small_65_to_127(
   byte const* _Nonnull p_source, byte* _Nonnull p_destination, idx bytes
) {
   char const* p_src = reinterpret_cast<char const*>(p_source);
   char* p_dest = reinterpret_cast<char*>(p_destination);
   char1x16 head_first;
   char1x16 head_second;
   char1x16 head_third;
   char1x16 head_fourth;
   char1x16 tail_first;
   char1x16 tail_second;
   char1x16 tail_third;
   char1x16 tail_fourth;
   head_first.load_unaligned(p_src);
   head_second.load_unaligned(p_src + 16);
   head_third.load_unaligned(p_src + 32);
   head_fourth.load_unaligned(p_src + 48);
   tail_first.load_unaligned(p_src + bytes - 64);
   tail_second.load_unaligned(p_src + bytes - 48);
   tail_third.load_unaligned(p_src + bytes - 32);
   tail_fourth.load_unaligned(p_src + bytes - 16);
   head_first.store_unaligned(p_dest);
   head_second.store_unaligned(p_dest + 16);
   head_third.store_unaligned(p_dest + 32);
   head_fourth.store_unaligned(p_dest + 48);
   tail_first.store_unaligned(p_dest + bytes - 64);
   tail_second.store_unaligned(p_dest + bytes - 48);
   tail_third.store_unaligned(p_dest + bytes - 32);
   tail_fourth.store_unaligned(p_dest + bytes - 16);
}

[[clang::no_builtin("memmove")]]
inline void
copy_memory_backward_small(
   byte const* _Nonnull p_source, byte* _Nonnull p_destination, idx bytes
) {
   if (bytes == 0u) {
      return;
   }
   if (bytes < 4u) {
      copy_memory_backward_small_1_to_3(p_source, p_destination, bytes);
      return;
   }
   if (bytes < 8u) {
      copy_memory_backward_small_4_to_7(p_source, p_destination, bytes);
      return;
   }
   if (bytes < 16u) {
      copy_memory_backward_small_8_to_15(p_source, p_destination, bytes);
      return;
   }
   if (bytes <= 32u) {
      copy_memory_backward_small_16_to_32(p_source, p_destination, bytes);
      return;
   }
   if (bytes <= 64u) {
      copy_memory_backward_small_33_to_64(p_source, p_destination, bytes);
      return;
   }
   copy_memory_backward_small_65_to_127(p_source, p_destination, bytes);
}

}  // namespace cat::detail
