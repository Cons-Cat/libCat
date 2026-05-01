// -*- mode: c++ -*-
#pragma once

// Layered SIMD helpers on `<cat/simd>`. Elementwise functors and factories
// (`simd_sqrt`, `make_simd_loaded`, `simd_filled`, ...) live in
// `<cat/simd_ops>`. Include that header when you need them. `<cat/simd>` does
// not pull it in.
//
// Chunk iterators (`as_vectorized`) live in `<cat/simd_iterator>`. Per-lane
// range iteration (P3480) lives on `<cat/simd>` (`begin` / `end`,
// `default_sentinel_t`).
//
// This header keeps lane algorithms (`simd_concat`, `simd_resize`,
// `simd_insert`, `simd_extract`, ...), lane builtins (`simd_clz`, ...), P3299
// pointer helpers, `simd_chunked_invoke`, and includes x86 ABI specialization
// headers at the end.

namespace cat {

template <is_simd Simd>
[[nodiscard]]
constexpr auto
simd_concat(Simd const& lower_lanes_first, Simd const& upper_lanes_second) {
   // P2638R0 glue smaller packs into one (`concat` / split story).
   // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2638r0.pdf
   using T = typename Simd::scalar_type;
   using abi = typename Simd::abi_type;
   using out_abi = fixed_size_abi<T, idx{abi::lanes.raw * 2uz}>;
   simd<T, out_abi> result{};
   for (idx i = 0u; i < abi::lanes; ++i) {
      result.set_lane(i, lower_lanes_first[i]);
      result.set_lane(i + abi::lanes, upper_lanes_second[i]);
   }
   return result;
}

template <idx new_lanes, typename Simd>
   requires((is_simd<Simd> || is_simd_mask<Simd>) && new_lanes != 0)
[[nodiscard]]
constexpr auto
simd_resize(Simd const& v) -> resize_simd<new_lanes, Simd> {
   // P2638R0 resize to `new_lanes` (truncate or value-init new lanes).
   // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2638r0.pdf
   resize_simd<new_lanes, Simd> result{};
   idx const src_lanes = Simd::abi_type::lanes;
   for (idx i = 0u; i < new_lanes && i < src_lanes; ++i) {
      result.set_lane(i, v[i]);
   }
   return result;
}

template <idx begin_index, idx end_index, typename Simd>
   requires(is_simd_or_mask<Simd> && end_index > begin_index
            && end_index <= Simd::abi_type::lanes)
[[nodiscard]]
constexpr auto
simd_extract(Simd const& parent)
   -> resize_simd<idx{end_index.raw - begin_index.raw}, Simd> {
   // P2638R0 `extract` a contiguous lane subrange `[begin_index, end_index)`.
   // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2638r0.pdf
   resize_simd<idx{end_index.raw - begin_index.raw}, Simd> result{};
   for (idx i = begin_index; i < end_index; ++i) {
      result.set_lane(idx(i.raw - begin_index.raw), parent[i]);
   }
   return result;
}

template <idx position, typename SimdParent, typename SimdChild>
   requires(is_simd_or_mask<SimdParent> && is_simd_or_mask<SimdChild>
            && is_same<typename SimdParent::scalar_type,
                       typename SimdChild::scalar_type>
            && position.raw + SimdChild::abi_type::lanes.raw
                  <= SimdParent::abi_type::lanes.raw)
[[nodiscard]]
constexpr auto
simd_insert(SimdParent const& parent, SimdChild const& child) -> SimdParent {
   // P2638R0 `insert` a smaller pack into a lane range of the parent.
   // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2638r0.pdf
   SimdParent result = parent;
   for (idx i = 0u; i < SimdChild::abi_type::lanes; ++i) {
      result.set_lane(position + i, child[i]);
   }
   return result;
}

template <is_simd Simd>
[[nodiscard]]
constexpr auto
simd_interleave(Simd const& from_even_lane_index,
                Simd const& from_odd_lane_index) {
   using T = typename Simd::scalar_type;
   using abi = typename Simd::abi_type;
   using out_abi = fixed_size_abi<T, idx{abi::lanes.raw * 2uz}>;
   simd<T, out_abi> result{};
   for (idx i = 0u; i < abi::lanes; ++i) {
      result.set_lane(idx(i.raw * 2uz), from_even_lane_index[i]);
      result.set_lane(idx(i.raw * 2uz + 1uz), from_odd_lane_index[i]);
   }
   return result;
}

template <is_simd Simd>
[[nodiscard]]
constexpr auto
simd_duplicate_even(Simd input) -> Simd {
   Simd result{};
   for (idx i = 0u; i < Simd::abi_type::lanes; ++i) {
      idx const from = idx(i.raw & ~static_cast<idx::raw_type>(1u));
      result.set_lane(i, input[from]);
   }
   return result;
}

template <is_simd Simd>
[[nodiscard]]
constexpr auto
simd_duplicate_odd(Simd input) -> Simd {
   Simd result{};
   for (idx i = 0u; i < Simd::abi_type::lanes; ++i) {
      idx::raw_type raw = i.raw | static_cast<idx::raw_type>(1u);
      if (raw >= Simd::abi_type::lanes.raw) {
         raw = Simd::abi_type::lanes.raw - 1u;
      }
      result.set_lane(i, input[idx(raw)]);
   }
   return result;
}

template <is_simd Simd>
[[nodiscard]]
constexpr auto
simd_reverse_blocks(Simd input, idx block_lanes) -> Simd {
   using abi = typename Simd::abi_type;
   if (block_lanes == 0u || (abi::lanes % block_lanes) != 0u) {
      return input;
   }
   Simd result{};
   for (idx base = 0u; base < abi::lanes; base += block_lanes) {
      for (idx j = 0u; j < block_lanes; ++j) {
         idx const tail =
            idx(block_lanes.raw - static_cast<idx::raw_type>(1u) - j.raw);
         result.set_lane(base + j, input[base + tail]);
      }
   }
   return result;
}

template <typename T, typename Abi>
   requires(is_integral<T> && !is_bool<T>)
[[nodiscard]]
constexpr auto
simd_clz(simd<T, Abi> x) -> simd<T, Abi> {
   return simd<T, Abi>(__builtin_elementwise_clzg(x.raw));
}

template <typename T, typename Abi>
   requires(is_integral<T> && !is_bool<T>)
[[nodiscard]]
constexpr auto
simd_clz(simd<T, Abi> x, simd<T, Abi> if_zero) -> simd<T, Abi> {
   return simd<T, Abi>(__builtin_elementwise_clzg(x.raw, if_zero.raw));
}

template <typename T, typename Abi>
   requires(is_integral<T> && !is_bool<T>)
[[nodiscard]]
constexpr auto
simd_ctz(simd<T, Abi> x) -> simd<T, Abi> {
   return simd<T, Abi>(__builtin_elementwise_ctzg(x.raw));
}

template <typename T, typename Abi>
   requires(is_integral<T> && !is_bool<T>)
[[nodiscard]]
constexpr auto
simd_ctz(simd<T, Abi> x, simd<T, Abi> if_zero) -> simd<T, Abi> {
   return simd<T, Abi>(__builtin_elementwise_ctzg(x.raw, if_zero.raw));
}

// P3299-style free functions (pointer and `idx` length). The members live on
// `simd`.
template <is_simd Simd>
constexpr auto
simd_partial_load(Simd& v, typename Simd::memory_lane const* p, idx n)
   -> Simd& {
   return v.partial_load(p, n);
}

template <is_simd Simd>
constexpr void
simd_partial_store(Simd const& v, typename Simd::memory_lane* p, idx n) {
   v.partial_store(p, n);
}

template <typename T, typename Abi>
[[nodiscard]]
constexpr auto
simd_mask_none_of(simd_mask<T, Abi> m) -> bool {
   // P2638R0 mask summaries (`none` via `simd_any_of`).
   // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2638r0.pdf
   return !simd_any_of(m);
}

template <typename T, typename Abi>
[[nodiscard]]
constexpr auto
simd_mask_all_of(simd_mask<T, Abi> m) -> bool {
   // P2638R0 forwards `simd_all_of` (`bitset`-style `all`).
   // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2638r0.pdf
   return simd_all_of(m);
}

template <typename T, typename Abi>
[[nodiscard]]
constexpr auto
simd_mask_any_of(simd_mask<T, Abi> m) -> bool {
   // P2638R0 forwards `simd_any_of` (`bitset`-style `any`).
   // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2638r0.pdf
   return simd_any_of(m);
}

template <typename Fn, typename T, typename Abi, typename... Args>
constexpr void
simd_chunked_invoke(Fn&& fn, simd<T, Abi> const& pack, Args&&... arguments) {
   $fwd(fn)(pack, $fwd(arguments)...);
}

}  // namespace cat

// x86 helpers ship with `<cat/simd>` (`simd_sse2_movmsk.hpp`,
// `simd_avx2_mask_ops.hpp`, ...). SSE4.2 string helpers live in
// `simd_sse42.hpp`.
#include <cat/detail/simd_sse42.hpp>
