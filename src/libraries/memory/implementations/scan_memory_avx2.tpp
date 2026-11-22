// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/bit>
#include <cat/simd>

#include "scan_memory_chunk.tpp"

namespace cat::detail {

// Compare `[0, byte_count)` in strided `Simd` chunks.
template <memory_scan scan, typename Simd>
[[nodiscard, gnu::always_inline]]
inline auto
scan_memory_strided(
   char const* _Nonnull p_left, char const* _Nonnull p_right, idx byte_count
) -> memory_scan_result<scan> {
   constexpr idx simd_size = sizeof(Simd);
   // `byte_count` is at least `simd_size`. The last chunk ends at
   // `byte_count`, overlapping the chunk before it.
   idx const tail = idx(byte_count - simd_size);
   if constexpr (scan == memory_scan::mismatch) {
      for (idx offset = 0u; offset < tail; offset += simd_size) {
         if (
            maybe<idx> const result = scan_memory_compare_chunk<Simd>(
               p_left + offset, p_right + offset
            );
            result.has_value()
         ) {
            return result.value() + offset;
         }
      }

      return scan_memory_at<scan>(
         scan_memory_compare_chunk<Simd>(p_left + tail, p_right + tail),
         tail
      );
   } else {
      auto equal =
         scan_memory_equal_lanes<Simd>(p_left + tail, p_right + tail);
      for (idx offset = 0u; offset < tail; offset += simd_size) {
         equal &= scan_memory_equal_lanes<Simd>(
            p_left + offset, p_right + offset
         );
      }

      return !equal.all_of();
   }
}

// Compare sizes `17..255` with AVX2.
template <memory_scan scan>
[[nodiscard, clang::no_builtin("memcmp"), gnu::target("avx2")]]
inline auto
scan_memory_avx2(
   byte const* _Nonnull p_lhs, byte const* _Nonnull p_rhs, idx byte_count
) -> memory_scan_result<scan> {
   char const* _Nonnull const p_left =
      reinterpret_cast<char const* _Nonnull>(p_lhs);
   char const* _Nonnull const p_right =
      reinterpret_cast<char const* _Nonnull>(p_rhs);

   if (byte_count <= 32u) {
      return scan_memory_chunks<scan, x64::sse_simd<char>>(
         p_left, p_right, array{0_idx, idx(byte_count - 16u)}
      );
   }

   return scan_memory_strided<scan, x64::avx_simd<char>>(
      p_left, p_right, byte_count
   );
}

}  // namespace cat::detail
