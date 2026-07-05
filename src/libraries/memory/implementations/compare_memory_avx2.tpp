// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/bit>
#include <cat/simd>

#include "compare_memory_chunk.tpp"

namespace cat::detail {

// Head+tail overlap compare for sizes `17..127` using 32-byte AVX2 chunks.
[[nodiscard, clang::no_builtin("memcmp"), gnu::target("avx2")]]
inline auto
compare_memory_avx2(byte const* _Nonnull p_lhs, byte const* _Nonnull p_rhs,
                    idx bytes) -> std::strong_ordering {
   char const* _Nonnull const p_left =
      reinterpret_cast<char const* _Nonnull>(p_lhs);
   char const* _Nonnull const p_right =
      reinterpret_cast<char const* _Nonnull>(p_rhs);
   idx const byte_count = bytes;

   if (byte_count <= 32u) {
      if (auto result =
             compare_memory_compare_chunk<x64::sse_simd<char>>(p_left, p_right);
          result != std::strong_ordering::equal) {
         return result;
      }
      return compare_memory_compare_chunk<x64::sse_simd<char>>(
         p_left + byte_count - 16, p_right + byte_count - 16);
   }

   if (auto result =
          compare_memory_compare_chunk<x64::avx_simd<char>>(p_left, p_right);
       result != std::strong_ordering::equal) {
      return result;
   }
   if (byte_count < 64u) {
      return compare_memory_compare_chunk<x64::avx_simd<char>>(
         p_left + byte_count - 32, p_right + byte_count - 32);
   }

   if (auto result = compare_memory_compare_chunk<x64::avx_simd<char>>(
          p_left + 32, p_right + 32);
       result != std::strong_ordering::equal) {
      return result;
   }
   if (byte_count < 96u) {
      return compare_memory_compare_chunk<x64::avx_simd<char>>(
         p_left + byte_count - 32, p_right + byte_count - 32);
   }

   if (auto result = compare_memory_compare_chunk<x64::avx_simd<char>>(
          p_left + 64, p_right + 64);
       result != std::strong_ordering::equal) {
      return result;
   }
   if (byte_count <= 127u) {
      return compare_memory_compare_chunk<x64::avx_simd<char>>(
         p_left + byte_count - 32, p_right + byte_count - 32);
   }

   for (idx offset = 96u; offset + 32u <= byte_count - 32u; offset += 32u) {
      if (auto result = compare_memory_compare_chunk<x64::avx_simd<char>>(
             p_left + offset, p_right + offset);
          result != std::strong_ordering::equal) {
         return result;
      }
   }
   return compare_memory_compare_chunk<x64::avx_simd<char>>(
      p_left + byte_count - 32, p_right + byte_count - 32);
}

}  // namespace cat::detail
