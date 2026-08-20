#pragma once

#include <cat/arithmetic>
#include <cat/bit>
#include <cat/meta>

#include "cat/detail/simd_sse_movmsk.hpp"

namespace x64::detail {

[[nodiscard]]
inline auto
sse2_movmsk_full_lane_mask(cat::idx lane_count) -> __UINT32_TYPE__ {
   return (1u << lane_count) - 1u;
}

template <typename T, x64::is_sse_abi<T> Abi>
[[nodiscard, gnu::target("sse2"), gnu::always_inline, gnu::nodebug]]
constexpr auto
sse2_abi_masked_lane_bits(cat::simd_mask<T, Abi> const& mask)
   -> __UINT32_TYPE__ {
   return sse2_abi_mask_to_bitset(mask)
          & sse2_movmsk_full_lane_mask(Abi::lanes);
}

}  // namespace x64::detail

namespace cat::detail::simd_abi {

template <typename T, x64::is_sse_abi<T> Abi>
struct mask_count_if_true<T, Abi> {
   [[nodiscard, gnu::target("sse2"), gnu::nodebug]]
   static constexpr auto
   invoke(simd_mask<T, Abi> const& mask) -> idx {
      return idx{
         __builtin_popcountg(x64::detail::sse2_abi_masked_lane_bits(mask))
      };
   }
};

template <typename T, x64::is_sse_abi<T> Abi>
struct mask_find_if_true<T, Abi> {
   [[nodiscard, gnu::target("sse2"), gnu::nodebug]]
   static constexpr auto
   invoke(simd_mask<T, Abi> const& mask) -> idx {
      return countr_zero(x64::detail::sse2_abi_masked_lane_bits(mask));
   }
};

template <typename T, x64::is_sse_abi<T> Abi>
struct mask_find_last_if_true<T, Abi> {
   [[nodiscard, gnu::target("sse2"), gnu::nodebug]]
   static constexpr auto
   invoke(simd_mask<T, Abi> const& mask) -> idx {
      return 31u - countl_zero(x64::detail::sse2_abi_masked_lane_bits(mask));
   }
};

template <typename T, x64::is_sse_abi<T> Abi>
struct mask_all_of<T, Abi> {
   [[nodiscard, gnu::target("sse2"), gnu::nodebug]]
   static constexpr auto
   invoke(simd_mask<T, Abi> const& mask) -> bool {
      return x64::detail::sse2_abi_masked_lane_bits(mask)
             == x64::detail::sse2_movmsk_full_lane_mask(Abi::lanes);
   }
};

template <typename T, x64::is_sse_abi<T> Abi>
struct mask_any_of<T, Abi> {
   [[nodiscard, gnu::target("sse2"), gnu::nodebug]]
   static constexpr auto
   invoke(simd_mask<T, Abi> const& mask) -> bool {
      return x64::detail::sse2_abi_masked_lane_bits(mask) != 0u;
   }
};

}  // namespace cat::detail::simd_abi
