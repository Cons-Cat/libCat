// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// Included from `global_includes.hpp` only after `<cat/maybe>` and
// `<cat/runtime>` so `bitset` and `basic_int<>::to_idx()` are available. Do
// not include from `<cat/simd>` to avoid pulling `bitset` while `maybe` is
// still being parsed.
//
// `mask_to_bitset` for x86 ABIs lives here so `<cat/simd>` does not pull
// `<cat/bitset>` during its initial parse. Movmsk helpers live in
// `simd_avx2_mask_ops.hpp`/`simd_sse_movmsk.hpp`. That keeps `<cat/memory>` ->
// `<cat/simd>` -> `implementations/simd_abi_*.tpp` from reaching
// `<cat/container>` while outer `<cat/maybe>` has not finished defining
// `nullopt`/`.to_idx()`.
// TODO: Rethink this when `.to_idx()` is removed.

#include <cat/detail/simd_abi_hooks.hpp>
#include <cat/detail/simd_switch_priority.hpp>

#include <cat/bitset>
#include <cat/memory>
#include <cat/simd>

namespace cat::detail::simd_abi {

template <typename T, x64::is_avx_abi<T> Abi>
struct mask_to_bitset<T, Abi> {
   [[nodiscard, gnu::target("avx2")]]
   static constexpr auto
   invoke(simd_mask<T, Abi> mask) -> __UINT64_TYPE__ {
      // P2638R0 pack mask lanes into a lane-bit pattern for `bitset`.
      // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2638r0.pdf
      return static_cast<__UINT64_TYPE__>(
         x64::detail::avx2_abi_mask_to_bitset(mask)
      );
   }
};

template <typename T, x64::is_sse_abi<T> Abi>
struct mask_to_bitset<T, Abi> {
   [[nodiscard, gnu::target("sse2")]]
   static constexpr auto
   invoke(simd_mask<T, Abi> mask) -> __UINT64_TYPE__ {
      // P2638R0 pack mask lanes into a lane-bit pattern for `bitset`.
      // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2638r0.pdf
      return static_cast<__UINT64_TYPE__>(
         x64::detail::sse2_abi_mask_to_bitset(mask)
      );
   }
};

}  // namespace cat::detail::simd_abi

namespace cat {

namespace detail {

template <typename Abi, typename T>
inline constexpr bool is_fixed_size_mask_abi = false;

template <typename T, idx lane_count>
inline constexpr bool
   is_fixed_size_mask_abi<cat::simd_abi::fixed_size<T, lane_count>, T> = true;

template <typename T, idx lane_count>
inline constexpr bool is_fixed_size_mask_abi<
   cat::simd_abi::unaligned<cat::simd_abi::fixed_size<T, lane_count>>, T> =
   true;

template <typename T, typename Abi>
   requires(is_fixed_size_mask_abi<Abi, T>)
[[nodiscard, gnu::target("avx2")]]
auto
fixed_size_mask_to_bitset_avx2(simd_mask<T, Abi> const& mask)
   -> bitset<Abi::lanes> {
   bitset<Abi::lanes> out{};
   using native_abi = x64::avx_abi<T>;
   using native_mask = simd_mask<T, native_abi>;
   constexpr idx logical_word_bits = sizeof(__UINT64_TYPE__) * 8u;
   constexpr idx logical_word_count = div_ceil(Abi::lanes, logical_word_bits);
   array<__UINT64_TYPE__, logical_word_count> logical_words{};

   for (idx lane_offset = 0u; lane_offset < Abi::lanes;
        lane_offset += native_abi::lanes) {
      idx const remaining{Abi::lanes.raw - lane_offset.raw};
      idx const chunk_lanes = min(remaining, native_abi::lanes);
      native_mask chunk(typename native_mask::raw_type{});
      copy_memory(
         reinterpret_cast<unsigned char const*>(&mask.raw)
            + (lane_offset.raw * sizeof(T)),
         &chunk.raw, chunk_lanes.raw * sizeof(T)
      );

      __UINT64_TYPE__ const pattern =
         x64::detail::avx2_abi_mask_to_bitset(chunk);
      idx const word_index = lane_offset / logical_word_bits;
      idx const word_offset = lane_offset % logical_word_bits;
      logical_words[word_index] |= pattern << word_offset.raw;
      if (word_offset != 0u && word_offset + chunk_lanes > logical_word_bits) {
         logical_words[word_index + 1u] |=
            pattern >> (logical_word_bits - word_offset).raw;
      }
   }

   constexpr idx final_word_lanes = Abi::lanes % logical_word_bits;
   if constexpr (final_word_lanes != 0u) {
      logical_words[logical_word_count - 1u] &=
         (static_cast<__UINT64_TYPE__>(1u) << final_word_lanes.raw) - 1u;
   }

   if constexpr (Abi::lanes > logical_word_bits) {
      using result_type = bitset<Abi::lanes>;
      array<typename result_type::storage_type, logical_word_count>
         storage_words{};
      constexpr idx padding =
         logical_word_count * logical_word_bits - Abi::lanes;
      for (idx i = 0u; i < logical_word_count; ++i) {
         idx const storage_index{logical_word_count.raw - 1u - i.raw};
         storage_words[storage_index] |= typename result_type::storage_type{
            logical_words[i] << padding.raw,
         };
         if constexpr (padding != 0u) {
            if (storage_index != 0u) {
               storage_words[idx{storage_index.raw - 1u}] |=
                  typename result_type::storage_type{
                     logical_words[i] >> (logical_word_bits - padding).raw,
               };
            }
         }
      }
      return [&]<idx... indices>(index_list_type<indices...>) {
         return make_bitset<Abi::lanes>(storage_words[indices]...);
      }(make_index_sequence<logical_word_count>{});
   } else {
      for (idx i = 0u; i < Abi::lanes; ++i) {
         out[i] = ((logical_words[0u] >> i.raw) & 1u) != 0u;
      }
      return out;
   }
}

template <typename T, typename Abi>
   requires(is_fixed_size_mask_abi<Abi, T>)
[[nodiscard]]
constexpr auto
fixed_size_mask_to_bitset(simd_mask<T, Abi> const& mask) -> bitset<Abi::lanes> {
   if !consteval {
      if (simd_dispatch_priority >= 80) {
         bitset<Abi::lanes> result = fixed_size_mask_to_bitset_avx2(mask);
         x64::zero_upper_avx_registers();
         return result;
      }
   }

   bitset<Abi::lanes> out{};
   for (idx i = 0u; i < Abi::lanes; ++i) {
      out[i] = mask[i];
   }
   return out;
}

}  // namespace detail

template <typename T, typename Abi>
[[nodiscard]]
constexpr auto
simd_to_bitset(simd_mask<T, Abi> mask) -> bitset<Abi::lanes> {
   // P2638R0 `simd_mask` as `bitset` for lane predicates.
   // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2638r0.pdf
   if constexpr (detail::is_fixed_size_mask_abi<Abi, T>) {
      return detail::fixed_size_mask_to_bitset(mask);
   } else if constexpr (requires {
                           detail::simd_abi::mask_to_bitset<T, Abi>::invoke(
                              mask
                           );
                        }) {
      using hook = detail::simd_abi::mask_to_bitset<T, Abi>;
      using mask_lane_bits =
         decltype(hook::invoke(declval<simd_mask<T, Abi>>()));

      static_assert(
         Abi::lanes <= sizeof(mask_lane_bits) * 8u,
         "`mask_to_bitset::invoke` for this ABI returns a type with fewer bits "
         "than there are lanes in `simd_mask` for this ABI."
      );

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
simd_mask<T, Abi>::to_bitset() const -> bitset<Abi::lanes> {
   // P2638R0 `simd_mask::to_bitset`.
   // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2638r0.pdf
   return simd_to_bitset(*this);
}

}  // namespace cat
