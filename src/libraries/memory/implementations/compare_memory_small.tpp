// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/bit>
#include <cat/simd>

namespace cat::detail {

[[nodiscard]]
inline auto
compare_memory_chunk(
   byte const* _Nonnull p_left, byte const* _Nonnull p_right, idx bytes
) -> std::strong_ordering {
   for (idx byte_index = 0u; byte_index < bytes; ++byte_index) {
      auto const left_byte = p_left[byte_index].value;
      auto const right_byte = p_right[byte_index].value;

      if (left_byte != right_byte) {
         return left_byte <=> right_byte;
      }
   }

   return std::strong_ordering::equal;
}

[[nodiscard]]
inline auto
compare_memory_word(byte const* _Nonnull p_left, byte const* _Nonnull p_right)
   -> std::strong_ordering {
   // Load the words.
   uword left_word;
   uword right_word;
   __builtin_memcpy_inline(&left_word, p_left, 8);
   __builtin_memcpy_inline(&right_word, p_right, 8);

   if (left_word == right_word) {
      return std::strong_ordering::equal;
   }

   // Find the first differing bit.
   uword const differing_bits = left_word ^ right_word;
   idx const byte_offset = is_target_little_endian
                              ? countr_zero(differing_bits) / char_bits
                              : countl_zero(differing_bits) / char_bits;

   return p_left[byte_offset].value <=> p_right[byte_offset].value;
}

// Power-of-2 branch tree for sizes `<= 32`.
[[nodiscard, clang::no_builtin("memcmp")]]
inline auto
compare_memory_small(
   byte const* _Nonnull p_lhs, byte const* _Nonnull p_rhs, idx bytes
) -> std::strong_ordering {
   byte const* _Nonnull const p_left = p_lhs;
   byte const* _Nonnull const p_right = p_rhs;
   idx const byte_count = bytes;

   if (byte_count == 0u) {
      return std::strong_ordering::equal;
   }

   if (byte_count == 1u) {
      return p_left[0].value <=> p_right[0].value;
   }

   auto compare_head_tail = [&]<idx chunk_bytes> {
      using word = uint_fixed<chunk_bytes>;

      if (byte_count == chunk_bytes) {
         word left_word;
         word right_word;
         __builtin_memcpy_inline(&left_word, p_left, chunk_bytes);
         __builtin_memcpy_inline(&right_word, p_right, chunk_bytes);

         if (left_word == right_word) {
            return std::strong_ordering::equal;
         }

         return compare_memory_chunk(p_left, p_right, chunk_bytes);
      }

      word left_head;
      word left_tail;
      word right_head;
      word right_tail;
      __builtin_memcpy_inline(&left_head, p_left, chunk_bytes);
      __builtin_memcpy_inline(
         &left_tail, p_left + byte_count - chunk_bytes, chunk_bytes
      );
      __builtin_memcpy_inline(&right_head, p_right, chunk_bytes);
      __builtin_memcpy_inline(
         &right_tail, p_right + byte_count - chunk_bytes, chunk_bytes
      );

      if (left_head != right_head) {
         return compare_memory_chunk(p_left, p_right, chunk_bytes);
      }

      if (left_tail != right_tail) {
         return compare_memory_chunk(
            p_left + byte_count - chunk_bytes,
            p_right + byte_count - chunk_bytes, chunk_bytes
         );
      }

      return std::strong_ordering::equal;
   };

   if (byte_count < 4u) {
      return compare_head_tail.template operator()<2u>();
   }
   if (byte_count < 8u) {
      return compare_head_tail.template operator()<4u>();
   }

   if (byte_count < 16u) {
      return compare_head_tail.template operator()<8u>();
   }

   if (byte_count == 16u) {
      if (
         auto result = compare_memory_word(p_left, p_right);
         result != std::strong_ordering::equal
      ) {
         return result;
      }
      return compare_memory_word(p_left + 8u, p_right + 8u);
   }

   if (
      auto result = compare_memory_word(p_left, p_right);
      result != std::strong_ordering::equal
   ) {
      return result;
   }

   if (
      auto result = compare_memory_word(p_left + 8u, p_right + 8u);
      result != std::strong_ordering::equal
   ) {
      return result;
   }

   if (
      auto result = compare_memory_word(
         p_left + byte_count - 16, p_right + byte_count - 16
      );
      result != std::strong_ordering::equal
   ) {
      return result;
   }

   return compare_memory_word(
      p_left + byte_count - 8, p_right + byte_count - 8
   );
}

}  // namespace cat::detail
