// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// Included from `global_includes.hpp` only after `<cat/maybe>` and
// `<cat/runtime>` so `bitset` and `arithmetic<>::to_idx()` are available. Do
// not include from `<cat/simd>` to avoid pulling `bitset` while `maybe` is
// still being parsed.
//
// `mask_to_bitset` for x86 ABIs lives here so `<cat/simd>` does not pull
// `<cat/bitset>` during its initial parse. Movmsk helpers live in
// `simd_avx2_mask_ops.hpp` / `simd_sse2_movmsk.hpp`. That keeps
// `<cat/memory>` → `<cat/simd>` → `implementations/simd_abi_*.tpp` from
// reaching `collection` while outer `<cat/maybe>` has not finished defining
// `nullopt` / `to_idx()`.

#include <cat/detail/simd_abi_hooks.hpp>

#include <cat/bitset>
#include <cat/simd>

namespace cat::detail::simd_abi {

template <typename T>
struct mask_to_bitset<T, x64::avx2_abi<T>> {
   [[nodiscard]]
   static constexpr auto
   invoke(simd_mask<T, x64::avx2_abi<T>> mask) -> __UINT64_TYPE__ {
      // P2638R0 pack mask lanes into a lane-bit pattern for `bitset`.
      // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2638r0.pdf
      return static_cast<__UINT64_TYPE__>(
         x64::detail::avx2_abi_mask_to_bitset(mask));
   }
};

template <typename T>
struct mask_to_bitset<T, x64::avx2_unaligned_abi<T>> {
   [[nodiscard]]
   static constexpr auto
   invoke(simd_mask<T, x64::avx2_unaligned_abi<T>> mask) -> __UINT64_TYPE__ {
      // P2638R0 pack mask lanes into a lane-bit pattern for `bitset`.
      // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2638r0.pdf
      return static_cast<__UINT64_TYPE__>(
         x64::detail::avx2_abi_mask_to_bitset(mask));
   }
};

template <typename T>
struct mask_to_bitset<T, x64::sse2_abi<T>> {
   [[nodiscard]]
   static constexpr auto
   invoke(simd_mask<T, x64::sse2_abi<T>> mask) -> __UINT64_TYPE__ {
      // P2638R0 pack mask lanes into a lane-bit pattern for `bitset`.
      // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2638r0.pdf
      return static_cast<__UINT64_TYPE__>(
         x64::detail::sse2_abi_mask_to_bitset(mask));
   }
};

template <typename T>
struct mask_to_bitset<T, x64::sse2_unaligned_abi<T>> {
   [[nodiscard]]
   static constexpr auto
   invoke(simd_mask<T, x64::sse2_unaligned_abi<T>> mask) -> __UINT64_TYPE__ {
      // P2638R0 pack mask lanes into a lane-bit pattern for `bitset`.
      // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2638r0.pdf
      return static_cast<__UINT64_TYPE__>(
         x64::detail::sse2_abi_mask_to_bitset(mask));
   }
};

}  // namespace cat::detail::simd_abi

namespace cat {

template <typename T, typename Abi>
[[nodiscard]]
constexpr auto
simd_to_bitset(simd_mask<T, Abi> mask) -> bitset<Abi::lanes> {
   // P2638R0 `simd_mask` as `bitset` for lane predicates.
   // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2638r0.pdf
   if constexpr (requires {
                    detail::simd_abi::mask_to_bitset<T, Abi>::invoke(mask);
                 }) {
      using hook = detail::simd_abi::mask_to_bitset<T, Abi>;
      using mask_lane_bits =
         decltype(hook::invoke(declval<simd_mask<T, Abi>>()));

      static_assert(
         Abi::lanes <= sizeof(mask_lane_bits) * 8u,
         "`mask_to_bitset::invoke` for this ABI returns a type with fewer bits "
         "than there are lanes in `simd_mask` for this ABI.");

      auto const pattern = detail::simd_abi::invoke<mask_lane_bits, hook>(mask);
      bitset<Abi::lanes> out{};
      for (idx i = 0u; i < Abi::lanes; ++i) {
         // TODO: `pattern >> i` should work when `i` is an `idx` right-hand
         // operand (`idx` has no bitwise operators of its own, so the shift
         // should fall through to the built-in on the integral `pattern`).
         // Today this is ambiguous because `idx` has multiple integral
         // conversion operators. Resolve the ambiguity and drop `.raw`.
         out[i] = ((pattern >> i.raw) & 1u) != 0u;
      }
      return out;
   } else {
      bitset<Abi::lanes> out{};
      for (idx i = 0u; i < Abi::lanes; ++i) {
         out[i] = mask[i];
      }
      return out;
   }
}

template <typename T, typename Abi>
constexpr auto
simd_mask<T, Abi>::to_bitset() const -> ::cat::bitset<Abi::lanes> {
   // P2638R0 `simd_mask::to_bitset`.
   // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2638r0.pdf
   return simd_to_bitset(*this);
}

}  // namespace cat
