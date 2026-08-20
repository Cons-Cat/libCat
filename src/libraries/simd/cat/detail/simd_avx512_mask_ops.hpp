#pragma once

#include <cat/arithmetic>
#include <cat/bit>
#include <cat/meta>

namespace x64::detail {

template <typename T, x64::is_avx512_abi<T> Abi>
[[nodiscard, gnu::always_inline]]
constexpr auto
avx512_abi_mask_to_bitset_consteval(cat::simd_mask<T, Abi> const& mask)
   -> __UINT64_TYPE__ {
   __UINT64_TYPE__ result = 0u;
   for (cat::idx i = 0u; i < Abi::lanes; ++i) {
      if (mask[i]) {
         result |= 1ull << i;
      }
   }
   return result;
}

template <typename T, x64::is_avx512_abi<T> Abi>
[[nodiscard, gnu::target("avx512f,avx512bw,avx512dq"), gnu::nodebug]]
inline auto
avx512_abi_mask_to_bitset(cat::simd_mask<T, Abi> mask) -> __UINT64_TYPE__ {
   if constexpr (sizeof(T) == 1u) {
      using raw = cat::simd<signed char, avx512_abi<signed char>>::raw_type;
      return static_cast<__UINT64_TYPE__>(
         __builtin_ia32_cvtb2mask512(__builtin_bit_cast(raw, mask.raw))
      );
   } else if constexpr (sizeof(T) == 2u) {
      using raw = cat::simd<short, avx512_abi<short>>::raw_type;
      return static_cast<__UINT64_TYPE__>(
         __builtin_ia32_cvtw2mask512(__builtin_bit_cast(raw, mask.raw))
      );
   } else if constexpr (sizeof(T) == 4u) {
      using raw = cat::simd<int, avx512_abi<int>>::raw_type;
      return static_cast<__UINT64_TYPE__>(
         __builtin_ia32_cvtd2mask512(__builtin_bit_cast(raw, mask.raw))
      );
   } else {
      using raw = cat::simd<long long, avx512_abi<long long>>::raw_type;
      return static_cast<__UINT64_TYPE__>(
         __builtin_ia32_cvtq2mask512(__builtin_bit_cast(raw, mask.raw))
      );
   }
}

[[nodiscard]]
inline auto
avx512_full_lane_mask(cat::idx lane_count) -> __UINT64_TYPE__ {
   if (lane_count == 64u) {
      return ~0ull;
   }
   return (1ull << lane_count) - 1ull;
}

template <typename T, x64::is_avx512_abi<T> Abi>
[[nodiscard, gnu::target("avx512f,avx512bw,avx512dq"), gnu::always_inline,
  gnu::nodebug]]
constexpr auto
avx512_abi_masked_lane_bits(cat::simd_mask<T, Abi> const& mask)
   -> __UINT64_TYPE__ {
   if consteval {
      return avx512_abi_mask_to_bitset_consteval(mask);
   }
   return avx512_abi_mask_to_bitset(mask) & avx512_full_lane_mask(Abi::lanes);
}

}  // namespace x64::detail

namespace cat::detail::simd_abi {

template <typename T, x64::is_avx512_abi<T> Abi>
struct mask_to_bitset<T, Abi> {
   [[nodiscard, gnu::target("avx512f,avx512bw,avx512dq"), gnu::nodebug]]
   static constexpr auto
   invoke(simd_mask<T, Abi> mask) -> __UINT64_TYPE__ {
      if consteval {
         return x64::detail::avx512_abi_mask_to_bitset_consteval(mask);
      }
      return x64::detail::avx512_abi_mask_to_bitset(mask);
   }
};

template <typename T, x64::is_avx512_abi<T> Abi>
struct mask_count_if_true<T, Abi> {
   [[nodiscard, gnu::target("avx512f,avx512bw,avx512dq"), gnu::nodebug]]
   static constexpr auto
   invoke(simd_mask<T, Abi> const& mask) -> idx {
      return idx{
         __builtin_popcountg(x64::detail::avx512_abi_masked_lane_bits(mask))
      };
   }
};

template <typename T, x64::is_avx512_abi<T> Abi>
struct mask_find_if_true<T, Abi> {
   [[nodiscard, gnu::target("avx512f,avx512bw,avx512dq"), gnu::nodebug]]
   static constexpr auto
   invoke(simd_mask<T, Abi> const& mask) -> idx {
      return countr_zero(x64::detail::avx512_abi_masked_lane_bits(mask));
   }
};

template <typename T, x64::is_avx512_abi<T> Abi>
struct mask_find_last_if_true<T, Abi> {
   [[nodiscard, gnu::target("avx512f,avx512bw,avx512dq"), gnu::nodebug]]
   static constexpr auto
   invoke(simd_mask<T, Abi> const& mask) -> idx {
      return 63u - countl_zero(x64::detail::avx512_abi_masked_lane_bits(mask));
   }
};

template <typename T, x64::is_avx512_abi<T> Abi>
struct mask_all_of<T, Abi> {
   [[nodiscard, gnu::target("avx512f,avx512bw,avx512dq"), gnu::nodebug]]
   static constexpr auto
   invoke(simd_mask<T, Abi> const& mask) -> bool {
      return x64::detail::avx512_abi_masked_lane_bits(mask)
             == x64::detail::avx512_full_lane_mask(Abi::lanes);
   }
};

template <typename T, x64::is_avx512_abi<T> Abi>
struct mask_any_of<T, Abi> {
   [[nodiscard, gnu::target("avx512f,avx512bw,avx512dq"), gnu::nodebug]]
   static constexpr auto
   invoke(simd_mask<T, Abi> const& mask) -> bool {
      return x64::detail::avx512_abi_masked_lane_bits(mask) != 0u;
   }
};

}  // namespace cat::detail::simd_abi
