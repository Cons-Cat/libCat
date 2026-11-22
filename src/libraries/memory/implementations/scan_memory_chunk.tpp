// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/array>
#include <cat/bit>
#include <cat/bitset>
#include <cat/simd>

namespace cat::detail {

enum class memory_scan : unsigned char {
   equality,  // True/false if any bytes differ.
   mismatch,  // Index of differing bytes.
};

// Whether the ranges differ for `equality`, or where they first differ for
// `mismatch`.
template <memory_scan scan>
using memory_scan_result =
   conditional<scan == memory_scan::mismatch, maybe<idx>, bool>;

template <memory_scan scan>
[[nodiscard, gnu::always_inline]]
constexpr auto
memory_scan_none() -> memory_scan_result<scan> {
   if constexpr (scan == memory_scan::mismatch) {
      return nullopt;
   } else {
      return false;
   }
}

template <memory_scan scan>
[[nodiscard, gnu::always_inline]]
constexpr auto
memory_scan_found(memory_scan_result<scan> const& result) -> bool {
   if constexpr (scan == memory_scan::mismatch) {
      return result.has_value();
   } else {
      return result;
   }
}

// Shift a result found in a chunk at `offset` to the whole range.
template <memory_scan scan>
[[nodiscard, gnu::always_inline]]
constexpr auto
scan_memory_at(memory_scan_result<scan> const& result, idx offset)
   -> memory_scan_result<scan> {
   if constexpr (scan == memory_scan::mismatch) {
      if (result.has_value()) {
         return result.value() + offset;
      }

      return nullopt;
   } else {
      return result;
   }
}

// The first lane at which two `Simd`s differ.
template <typename Simd>
[[nodiscard, gnu::always_inline]]
inline auto
scan_memory_mismatch_offset(Simd const& left_simd, Simd const& right_simd)
   -> maybe<idx> {
   auto const equal_mask = left_simd.equal_lanes(right_simd);
   if (equal_mask.all_of()) {
      return nullopt;
   }

   return equal_mask.to_bitset().countr_one();
}

template <typename Simd>
[[nodiscard, gnu::always_inline]]
inline auto
scan_memory_equal_lanes(
   char const* _Nonnull p_left, char const* _Nonnull p_right
) {
   Simd left_simd;
   Simd right_simd;
   left_simd.load(p_left);
   right_simd.load(p_right);
   return left_simd.equal_lanes(right_simd);
}

template <typename Simd>
[[nodiscard, gnu::always_inline]]
inline auto
scan_memory_compare_chunk(
   char const* _Nonnull p_left, char const* _Nonnull p_right
) -> maybe<idx> {
   Simd left_simd;
   Simd right_simd;
   left_simd.load(p_left);
   right_simd.load(p_right);
   return scan_memory_mismatch_offset(left_simd, right_simd);
}

// Compare `Simd` chunks at increasing `offsets`.
template <memory_scan scan, typename Simd, idx count>
[[nodiscard, gnu::always_inline]]
inline auto
scan_memory_chunks(
   char const* _Nonnull p_left, char const* _Nonnull p_right,
   array<idx, count> const& offsets
) -> memory_scan_result<scan> {
   if constexpr (scan == memory_scan::mismatch) {
      // Overlap with earlier chunks is already equal, so the first differing
      // chunk holds the first mismatch.
      for (idx const offset : offsets) {
         if (
            maybe<idx> const result = scan_memory_compare_chunk<Simd>(
               p_left + offset, p_right + offset
            );
            result.has_value()
         ) {
            return result.value() + offset;
         }
      }

      return nullopt;
   } else {
      // Bit-and every chunk's lanes to branch once.
      auto equal = scan_memory_equal_lanes<Simd>(
         p_left + offsets[0], p_right + offsets[0]
      );
      for (idx i = 1u; i < count; ++i) {
         equal &= scan_memory_equal_lanes<Simd>(
            p_left + offsets[i], p_right + offsets[i]
         );
      }

      return !equal.all_of();
   }
}

}  // namespace cat::detail
