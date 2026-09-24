#pragma once

#include <cat/arithmetic>
#include <cat/meta>
#include <cat/simd>

namespace x64::detail {

template <typename T, is_sse_abi<T> Abi>
[[nodiscard, gnu::target("sse2"), gnu::nodebug]]
constexpr auto
sse2_abi_mask_to_bitset(cat::simd_mask<T, Abi> mask) -> __UINT32_TYPE__ {
   if constexpr (sizeof(T) == 1) {
      return static_cast<__UINT32_TYPE__>(__builtin_ia32_pmovmskb128(mask.raw));
   } else if constexpr (sizeof(T) == 4) {
      return static_cast<__UINT32_TYPE__>(__builtin_ia32_movmskps(mask.raw));
   } else if constexpr (sizeof(T) == 8) {
      return static_cast<__UINT32_TYPE__>(__builtin_ia32_movmskpd(mask.raw));
   } else {
      // sizeof(T) == 2, lanes == 8. Saturating each 16-bit lane to a byte
      // keeps its sign bit, so one `pmovmskb` reads a bit per lane.
      using halves = short __attribute__((vector_size(16)));
      return static_cast<__UINT32_TYPE__>(
         __builtin_ia32_pmovmskb128(__builtin_ia32_packsswb128(
            __builtin_bit_cast(halves, mask.raw), halves{}
         ))
      );
   }
}

}  // namespace x64::detail
