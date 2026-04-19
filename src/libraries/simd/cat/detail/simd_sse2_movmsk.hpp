#pragma once

#include <cat/arithmetic>
#include <cat/meta>
#include <cat/simd>

namespace x64::detail {

template <typename T, typename Abi>
   requires(cat::is_same<typename Abi::scalar_type, T> && Abi::size == 16u
            && Abi::lanes == cat::idx{Abi::size.raw / sizeof(T)})
[[nodiscard]] constexpr auto
   sse2_abi_mask_to_bitset(cat::simd_mask<T, Abi> mask) -> __UINT32_TYPE__ {
   if constexpr (cat::is_same<T, float>) {
      return static_cast<__UINT32_TYPE__>(__builtin_ia32_movmskps(mask.raw));
   } else if constexpr (cat::is_same<T, double>) {
      return static_cast<__UINT32_TYPE__>(__builtin_ia32_movmskpd(mask.raw));
   } else if constexpr (sizeof(T) == 4 && Abi::lanes == 4) {
      return static_cast<__UINT32_TYPE__>(
         __builtin_ia32_movmskps(__builtin_bit_cast(
            typename cat::simd<float, Abi>::raw_type, mask.raw)));
   } else if constexpr (sizeof(T) == 8 && Abi::lanes == 2) {
      return static_cast<__UINT32_TYPE__>(
         __builtin_ia32_movmskpd(__builtin_bit_cast(
            typename cat::simd<double, Abi>::raw_type, mask.raw)));
   } else if constexpr (sizeof(T) == 1 && Abi::lanes == 16) {
      return static_cast<__UINT32_TYPE__>(__builtin_ia32_pmovmskb(mask.raw))
             & static_cast<__UINT32_TYPE__>(0xFFFF);
   } else {
      __UINT32_TYPE__ const u =
         static_cast<__UINT32_TYPE__>(__builtin_ia32_pmovmskb(mask.raw))
         & static_cast<__UINT32_TYPE__>(0xFFFF);
      constexpr cat::idx lane_count = Abi::lanes;
      constexpr cat::idx w = sizeof(T);
      __UINT32_TYPE__ out = 0;
      for (cat::idx i = 0u; i < lane_count; ++i) {
         __UINT32_TYPE__ t = 0;
         for (cat::idx j = 0u; j < w; ++j) {
            t |= (u >> (i.raw * w + j.raw)) & static_cast<__UINT32_TYPE__>(1);
         }
         if (t != 0) {
            out |= static_cast<__UINT32_TYPE__>(1) << i.raw;
         }
      }
      return out;
   }
}

}  // namespace x64::detail
