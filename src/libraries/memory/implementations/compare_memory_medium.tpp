// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/bit>
#include <cat/simd>

namespace cat::detail {

[[nodiscard, gnu::target("sse2"), gnu::always_inline]]
inline auto
compare_memory_chunk(char const* _Nonnull p_left, char const* _Nonnull p_right)
   -> std::strong_ordering {
   char1x16 left_chunk;
   char1x16 right_chunk;
   left_chunk.load_unaligned(p_left);
   right_chunk.load_unaligned(p_right);
   auto const equal_mask = left_chunk.equal_lanes(right_chunk);
   if (equal_mask.all_of()) {
      return std::strong_ordering::equal;
   }
   __UINT32_TYPE__ const equal_bits =
      x64::detail::sse2_abi_mask_to_bitset(equal_mask);
   __UINT32_TYPE__ const diff_bits = (~equal_bits) & 0xffffu;
   idx const byte_offset = countr_zero(diff_bits);
   return static_cast<unsigned char>(p_left[byte_offset])
          <=> static_cast<unsigned char>(p_right[byte_offset]);
}

// Head+tail overlap compare for sizes `33..127`, matching `copy_memory_small`.
[[nodiscard, clang::no_builtin("memcmp"), gnu::target("sse2")]]
inline auto
compare_memory_medium(byte const* _Nonnull p_lhs, byte const* _Nonnull p_rhs,
                      idx bytes) -> std::strong_ordering {
   char const* _Nonnull const p_left =
      reinterpret_cast<char const* _Nonnull>(p_lhs);
   char const* _Nonnull const p_right =
      reinterpret_cast<char const* _Nonnull>(p_rhs);
   idx const byte_count = bytes;

   if (byte_count < 64u) {
      if (auto result = compare_memory_chunk(p_left, p_right);
          result != std::strong_ordering::equal) {
         return result;
      }
      if (auto result = compare_memory_chunk(p_left + 16, p_right + 16);
          result != std::strong_ordering::equal) {
         return result;
      }
      if (auto result = compare_memory_chunk(p_left + byte_count - 32,
                                             p_right + byte_count - 32);
          result != std::strong_ordering::equal) {
         return result;
      }
      return compare_memory_chunk(p_left + byte_count - 16,
                                  p_right + byte_count - 16);
   }

   if (auto result = compare_memory_chunk(p_left, p_right);
       result != std::strong_ordering::equal) {
      return result;
   }
   if (auto result = compare_memory_chunk(p_left + 16, p_right + 16);
       result != std::strong_ordering::equal) {
      return result;
   }
   if (auto result = compare_memory_chunk(p_left + 32, p_right + 32);
       result != std::strong_ordering::equal) {
      return result;
   }
   if (auto result = compare_memory_chunk(p_left + 48, p_right + 48);
       result != std::strong_ordering::equal) {
      return result;
   }
   if (auto result = compare_memory_chunk(p_left + byte_count - 64,
                                          p_right + byte_count - 64);
       result != std::strong_ordering::equal) {
      return result;
   }
   if (auto result = compare_memory_chunk(p_left + byte_count - 48,
                                          p_right + byte_count - 48);
       result != std::strong_ordering::equal) {
      return result;
   }
   if (auto result = compare_memory_chunk(p_left + byte_count - 32,
                                          p_right + byte_count - 32);
       result != std::strong_ordering::equal) {
      return result;
   }
   return compare_memory_chunk(p_left + byte_count - 16,
                               p_right + byte_count - 16);
}

}  // namespace cat::detail
