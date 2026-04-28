// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/meta>

// AVX2-class reductions follow the usual Intel-style pattern. Reduce a ymm
// compare mask to a small lane-bit map with `movmsk*`, then treat that word
// like a fixed-width bitset (`popcount`, `ctz`, `clz`). AVX2 has no instruction
// that yields first or last active lane indices directly, so this conversion is
// the customary path.
//
// Included from `<cat/simd>` after `simd_mask` is complete.
// `<cat/detail/simd_avx2.hpp>` cannot own these alone because it includes
// `<cat/simd>` and must not appear before `simd_mask` is complete.

namespace x64::detail {

// Movmsk-derived lane-bit map for `cat::bitset`. Bit `i` matches
// `simd_mask::operator[](i)` for masks from elementwise compares. The same word
// feeds mask reductions below after masking to active lanes. Consumed by
// `mask_to_bitset` in `implementations/simd_mask_bitset.tpp`.

template <typename T, typename Abi>
   requires(cat::is_same<Abi, x64::avx2_abi<T>>
            || cat::is_same<Abi, x64::avx2_unaligned_abi<T>>)
[[nodiscard]]
inline auto
avx2_abi_mask_to_bitset(cat::simd_mask<T, Abi> mask) -> __UINT32_TYPE__ {
   if constexpr (cat::is_same<T, float>) {
      return static_cast<__UINT32_TYPE__>(__builtin_ia32_movmskps256(mask.raw));
   } else if constexpr (cat::is_same<T, double>) {
      return static_cast<__UINT32_TYPE__>(__builtin_ia32_movmskpd256(mask.raw));
   } else if constexpr (sizeof(T) == 4 && Abi::lanes == 8) {
      return static_cast<__UINT32_TYPE__>(
         __builtin_ia32_movmskps256(__builtin_bit_cast(
            typename cat::simd<float, x64::avx2_abi<float>>::raw_type,
            mask.raw)));
   } else if constexpr (sizeof(T) == 8 && Abi::lanes == 4) {
      return static_cast<__UINT32_TYPE__>(
         __builtin_ia32_movmskpd256(__builtin_bit_cast(
            typename cat::simd<double, x64::avx2_abi<double>>::raw_type,
            mask.raw)));
   } else if constexpr (sizeof(T) == 1 && Abi::lanes == 32) {
      return static_cast<__UINT32_TYPE__>(__builtin_ia32_pmovmskb256(mask.raw));
   } else {
      __UINT32_TYPE__ const pmovmskb_byte_bitmap =
         static_cast<__UINT32_TYPE__>(__builtin_ia32_pmovmskb256(mask.raw));
      constexpr cat::idx lane_count = Abi::lanes;
      constexpr cat::idx lane_element_byte_width = sizeof(T);
      __UINT32_TYPE__ logical_lane_bits = 0;
      for (cat::idx lane_index = 0u; lane_index < lane_count; ++lane_index) {
         __UINT32_TYPE__ lane_truth_aggregate = 0;
         for (cat::idx byte_offset_in_lane = 0u;
              byte_offset_in_lane < lane_element_byte_width;
              ++byte_offset_in_lane) {
            lane_truth_aggregate |=
               (pmovmskb_byte_bitmap
                >> (lane_index.raw * lane_element_byte_width.raw
                    + byte_offset_in_lane.raw))
               & static_cast<__UINT32_TYPE__>(1);
         }
         if (lane_truth_aggregate != 0) {
            logical_lane_bits |= static_cast<__UINT32_TYPE__>(1)
                                 << lane_index.raw;
         }
      }
      return logical_lane_bits;
   }
}

[[nodiscard]]
inline auto
avx2_movmsk_full_lane_mask(cat::idx lane_count) -> __UINT32_TYPE__ {
   if (lane_count >= 32u) {
      return static_cast<__UINT32_TYPE__>(~0u);
   }
   return static_cast<__UINT32_TYPE__>((1ull << lane_count.raw) - 1ull);
}

// Lane-bit pattern ANDed with active lanes. Reduction functors operate on this
// word rather than reopening movmsk separately because sharing
// `avx2_abi_mask_to_bitset` matches normal practice and keeps one place for odd
// lane layouts.
template <typename T, typename Abi>
   requires(cat::is_same<Abi, x64::avx2_abi<T>>
            || cat::is_same<Abi, x64::avx2_unaligned_abi<T>>)
[[nodiscard, gnu::always_inline, gnu::nodebug]]
inline constexpr auto
avx2_abi_masked_lane_bits(cat::simd_mask<T, Abi> const& mask)
   -> __UINT32_TYPE__ {
   return avx2_abi_mask_to_bitset(mask)
          & avx2_movmsk_full_lane_mask(Abi::lanes);
}

}  // namespace x64::detail

namespace cat::detail::simd_abi {

// Each specialization below reads the masked lane-bit word from
// `avx2_abi_masked_lane_bits` then runs scalar bit scans on it. That is the
// same customary AVX2 approach as MOVMSKPS plus BSF or BSR style operations in
// assembly references.

// TODO: Swap the fallbacks below back to the commented `__builtin_stdc_*`
// instrinsics once the Clang distribution at `apt.llvm.org` updates.

template <typename T, typename Abi>
   requires(cat::is_same<Abi, x64::avx2_abi<T>>
            || cat::is_same<Abi, x64::avx2_unaligned_abi<T>>)
struct mask_count_if_true<T, Abi> {
   [[nodiscard]]
   static constexpr auto
   invoke(simd_mask<T, Abi> const& mask) -> idx {
      // `popcount` on the movmsk lane-bit map.
      __UINT32_TYPE__ const masked_logical_lane_bits =
         ::x64::detail::avx2_abi_masked_lane_bits(mask);
      return idx{__builtin_popcountg(masked_logical_lane_bits)};
   }
};

template <typename T, typename Abi>
   requires(cat::is_same<Abi, x64::avx2_abi<T>>
            || cat::is_same<Abi, x64::avx2_unaligned_abi<T>>)
struct mask_find_if_true<T, Abi> {
   [[nodiscard]]
   static constexpr auto
   invoke(simd_mask<T, Abi> const& mask) -> idx {
      // At least one true lane (`ctz` is well-defined when the movmsk word is
      // non-zero).
      __UINT32_TYPE__ const masked_logical_lane_bits =
         ::x64::detail::avx2_abi_masked_lane_bits(mask);
      // return __builtin_stdc_trailing_zeros(masked_logical_lane_bits);
      return static_cast<unsigned>(__builtin_ctzg(masked_logical_lane_bits));
   }
};

template <typename T, typename Abi>
   requires(cat::is_same<Abi, x64::avx2_abi<T>>
            || cat::is_same<Abi, x64::avx2_unaligned_abi<T>>)
struct mask_find_last_if_true<T, Abi> {
   [[nodiscard]]
   static constexpr auto
   invoke(simd_mask<T, Abi> const& mask) -> idx {
      // At least one true lane (lane-bit word is non-zero).
      __UINT32_TYPE__ const masked_logical_lane_bits =
         ::x64::detail::avx2_abi_masked_lane_bits(mask);
      // return __builtin_stdc_bit_width(masked_logical_lane_bits) - 1u;
      return 31u - __builtin_clzg(masked_logical_lane_bits);
   }
};

template <typename T, typename Abi>
   requires(cat::is_same<Abi, x64::avx2_abi<T>>
            || cat::is_same<Abi, x64::avx2_unaligned_abi<T>>)
struct mask_all_of<T, Abi> {
   [[nodiscard]]
   static constexpr auto
   invoke(simd_mask<T, Abi> mask) -> bool {
      // Movmsk lane bits equal the active-lane mask iff every lane is true.
      return ::x64::detail::avx2_abi_masked_lane_bits(mask)
             == ::x64::detail::avx2_movmsk_full_lane_mask(Abi::lanes);
   }
};

template <typename T, typename Abi>
   requires(cat::is_same<Abi, x64::avx2_abi<T>>
            || cat::is_same<Abi, x64::avx2_unaligned_abi<T>>)
struct mask_any_of<T, Abi> {
   [[nodiscard]]
   static constexpr auto
   invoke(simd_mask<T, Abi> mask) -> bool {
      return ::x64::detail::avx2_abi_masked_lane_bits(mask) != 0u;
   }
};

}  // namespace cat::detail::simd_abi
