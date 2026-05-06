#pragma once

#include <cat/arithmetic>
#include <cat/meta>
#include <cat/simd>

namespace x64::detail {

template <typename T, is_sse2_abi<T> Abi>
[[nodiscard]]
constexpr auto
sse2_abi_mask_to_bitset(cat::simd_mask<T, Abi> mask) -> __UINT32_TYPE__ {
   auto raw =
      __builtin_bit_cast(typename cat::simd<T, Abi>::raw_type, mask.raw);

   if constexpr (sizeof(T) == 1) {
      return static_cast<__UINT32_TYPE__>(__builtin_ia32_pmovmskb(raw));
   } else if constexpr (sizeof(T) == 4) {
      return static_cast<__UINT32_TYPE__>(__builtin_ia32_movmskps(raw));
   } else if constexpr (sizeof(T) == 8) {
      return static_cast<__UINT32_TYPE__>(__builtin_ia32_movmskpd(raw));
   } else {
      // sizeof(T) == 2, lanes == 8. Collapse the byte-bitmap from `pmovmskb`
      // into one bit per logical lane.
      // TODO: Is this the most efficient solution?
      __UINT32_TYPE__ const bytes{__builtin_ia32_pmovmskb(raw)};
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

}  // namespace x64::detail
