// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/array>
#include <cat/bit>
#include <cat/simd>

#include "scan_memory_chunk.tpp"

namespace cat::detail {

// Head+tail overlap compare for sizes `33..127`, matching `copy_memory_small`.
template <memory_scan scan>
[[nodiscard, clang::no_builtin("memcmp"), gnu::target("sse2")]]
inline auto
scan_memory_medium(
   byte const* _Nonnull p_lhs, byte const* _Nonnull p_rhs, idx byte_count
) -> memory_scan_result<scan> {
   char const* _Nonnull const p_left =
      reinterpret_cast<char const* _Nonnull>(p_lhs);
   char const* _Nonnull const p_right =
      reinterpret_cast<char const* _Nonnull>(p_rhs);

   if (byte_count < 64u) {
      return scan_memory_chunks<scan, char1x16>(
         p_left, p_right,
         array{0_idx, 16_idx, idx(byte_count - 32u), idx(byte_count - 16u)}
      );
   }

   return scan_memory_chunks<scan, char1x16>(
      p_left, p_right,
      array{
         0_idx, 16_idx, 32_idx, 48_idx, idx(byte_count - 64u),
         idx(byte_count - 48u), idx(byte_count - 32u), idx(byte_count - 16u)
      }
   );
}

}  // namespace cat::detail
