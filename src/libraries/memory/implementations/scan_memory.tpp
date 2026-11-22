// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/array>
#include <cat/memory>
#include <cat/simd>
#include <cat/simd_switch>

#include "scan_memory_avx2.tpp"
#include "scan_memory_medium.tpp"
#include "scan_memory_small.tpp"

namespace cat::detail {

template <typename Simd>
[[nodiscard]]
inline auto
scan_memory_batch_all_equal(
   array<typename Simd::mask_type, 4u> const& equal_masks
) -> bool {
   // Bit-and the lanes so that only one mask is tested.
   return (equal_masks[0] & equal_masks[1] & equal_masks[2] & equal_masks[3])
      .all_of();
}

template <typename Simd>
[[nodiscard]]
inline auto
scan_memory_first_mismatch_in_batch(
   array<Simd, 4u> const& left_simds, array<Simd, 4u> const& right_simds
) -> maybe<idx> {
#pragma unroll 4
   for (idx simd_index = 0u; simd_index < 4u; ++simd_index) {
      if (
         maybe<idx> const result = scan_memory_mismatch_offset(
            left_simds[simd_index], right_simds[simd_index]
         );
         result.has_value()
      ) {
         return result.value() + (simd_index * sizeof(Simd));
      }
   }
   return nullopt;
}

// Compare sizes of at least four `Simd`s, four at a time.
template <memory_scan scan, typename Simd>
// We must inline to infer the `gnu::target`.
[[gnu::always_inline]]
auto
scan_memory_large(
   byte const* _Nonnull p_lhs, byte const* _Nonnull p_rhs, idx bytes
) noexcept(!__has_feature(cxx_exceptions)) -> memory_scan_result<scan> {
   char const* _Nonnull const p_left =
      reinterpret_cast<char const* _Nonnull>(p_lhs);
   char const* _Nonnull const p_right =
      reinterpret_cast<char const* _Nonnull>(p_rhs);

   constexpr idx simd_size = sizeof(Simd);
   idx offset = 0u;

   array<Simd, 4u> left_simds;
   array<Simd, 4u> right_simds;
   array<typename Simd::mask_type, 4u> equal_masks;

   while (bytes - offset >= simd_size * 4u) {
      for (idx simd_index = 0u; simd_index < 4u; ++simd_index) {
         idx const chunk = offset + (simd_index * simd_size);
         left_simds[simd_index].load(p_left + chunk);
         right_simds[simd_index].load(p_right + chunk);
         equal_masks[simd_index] =
            left_simds[simd_index].equal_lanes(right_simds[simd_index]);
      }

      if (scan_memory_batch_all_equal<Simd>(equal_masks)) {
         offset += simd_size * 4u;
         continue;
      }

      if constexpr (scan == memory_scan::equality) {
         return true;
      } else {
         return scan_memory_at<scan>(
            scan_memory_first_mismatch_in_batch(left_simds, right_simds), offset
         );
      }
   }

   if (offset == bytes) {
      return memory_scan_none<scan>();
   }

   // Fewer than four `Simd`s remain. Start at least one `Simd` before the
   // end, overlapping bytes that are already known to be equal.
   idx const start = min(offset, idx(bytes - simd_size));
   return scan_memory_at<scan>(
      scan_memory_strided<scan, Simd>(
         p_left + start, p_right + start, idx(bytes - start)
      ),
      start
   );
}

template <memory_scan scan>
[[nodiscard]]
auto
scan_memory(
   byte const* _Nonnull p_lhs, byte const* _Nonnull p_rhs, idx bytes
) noexcept(!__has_feature(cxx_exceptions)) -> memory_scan_result<scan> {
   if (bytes <= 32u) {
      return scan_memory_small<scan>(p_lhs, p_rhs, bytes);
   }

   if (bytes <= 255u) {
#ifndef CAT_NO_CPUID
      if (simd_dispatch_priority >= 80) {
         return scan_memory_avx2<scan>(p_lhs, p_rhs, bytes);
      }
#endif
      if (bytes <= 127u) {
         return scan_memory_medium<scan>(p_lhs, p_rhs, bytes);
      }
   }

   return $simd_switch($abi((avx2, sse2), {
      return scan_memory_large<scan, char1x_>(p_lhs, p_rhs, bytes);
   }));
}

}  // namespace cat::detail
