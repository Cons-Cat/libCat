// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/bit>
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

template <typename T, x64::is_avx2_abi<T> Abi>
[[nodiscard]]
inline auto
avx2_abi_mask_to_bitset(cat::simd_mask<T, Abi> mask) -> __UINT32_TYPE__ {
   auto raw =
      __builtin_bit_cast(typename cat::simd<T, Abi>::raw_type, mask.raw);

   if constexpr (cat::is_same<T, float>) {
      return static_cast<__UINT32_TYPE__>(__builtin_ia32_movmskps256(raw));
   } else if constexpr (cat::is_same<T, double>) {
      return static_cast<__UINT32_TYPE__>(__builtin_ia32_movmskpd256(raw));
   } else if constexpr (sizeof(T) == 1) {
      return static_cast<__UINT32_TYPE__>(__builtin_ia32_pmovmskb256(raw));
   } else if constexpr (sizeof(T) == 4) {
      return static_cast<__UINT32_TYPE__>(__builtin_ia32_movmskps256(raw));
   } else if constexpr (sizeof(T) == 8) {
      return static_cast<__UINT32_TYPE__>(__builtin_ia32_movmskpd256(raw));
   } else {
      // sizeof(T) == 2, lanes == 16. Collapse the byte-bitmap from `pmovmskb`
      // into one bit per logical lane.
      // TODO: Is this the most efficient solution?
      __UINT32_TYPE__ const bytes{__builtin_ia32_pmovmskb256(raw)};
      __UINT32_TYPE__ lane_bits = 0;
      for (cat::idx i = 0u; i < Abi::lanes; ++i) {
         cat::uint4 lane_bitset = 0;
         for (cat::idx j = 0u; j < sizeof(T); ++j) {
            lane_bitset |= (bytes >> (i * sizeof(T) + j)) & 1u;
         }
         if (lane_bitset != 0) {
            lane_bits |= 1u << i;
         }
      }
      return lane_bits;
   }
}

[[nodiscard]]
inline auto
avx2_movmsk_full_lane_mask(cat::idx lane_count) -> __UINT32_TYPE__ {
   if (lane_count >= 32u) {
      return ~0u;
   }
   return (1u << lane_count) - 1u;
}

// Lane-bit pattern ANDed with active lanes. Reduction functors operate on this
// word rather than reopening movmsk separately because sharing
// `avx2_abi_mask_to_bitset` matches normal practice and keeps one place for odd
// lane layouts.
template <typename T, x64::is_avx2_abi<T> Abi>
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

template <typename T, x64::is_avx2_abi<T> Abi>
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

template <typename T, x64::is_avx2_abi<T> Abi>
struct mask_find_if_true<T, Abi> {
   [[nodiscard]]
   static constexpr auto
   invoke(simd_mask<T, Abi> const& mask) -> idx {
      // At least one true lane (`ctz` is well-defined when the movmsk word is
      // non-zero).
      __UINT32_TYPE__ const masked_logical_lane_bits =
         ::x64::detail::avx2_abi_masked_lane_bits(mask);
      // return __builtin_stdc_trailing_zeros(masked_logical_lane_bits);
      return make_raw_arithmetic(countr_zero(masked_logical_lane_bits));
   }
};

template <typename T, x64::is_avx2_abi<T> Abi>
struct mask_find_last_if_true<T, Abi> {
   [[nodiscard]]
   static constexpr auto
   invoke(simd_mask<T, Abi> const& mask) -> idx {
      // At least one true lane (lane-bit word is non-zero).
      __UINT32_TYPE__ const masked_logical_lane_bits =
         ::x64::detail::avx2_abi_masked_lane_bits(mask);
      // return __builtin_stdc_bit_width(masked_logical_lane_bits) - 1u;
      return 31u - make_raw_arithmetic(countl_zero(masked_logical_lane_bits));
   }
};

template <typename T, x64::is_avx2_abi<T> Abi>
struct mask_all_of<T, Abi> {
   [[nodiscard]]
   static constexpr auto
   invoke(simd_mask<T, Abi> mask) -> bool {
      // Movmsk lane bits equal the active-lane mask iff every lane is true.
      return ::x64::detail::avx2_abi_masked_lane_bits(mask)
             == ::x64::detail::avx2_movmsk_full_lane_mask(Abi::lanes);
   }
};

template <typename T, x64::is_avx2_abi<T> Abi>
struct mask_any_of<T, Abi> {
   [[nodiscard]]
   static constexpr auto
   invoke(simd_mask<T, Abi> mask) -> bool {
      return ::x64::detail::avx2_abi_masked_lane_bits(mask) != 0u;
   }
};

}  // namespace cat::detail::simd_abi
