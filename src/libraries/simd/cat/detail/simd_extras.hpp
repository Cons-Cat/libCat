// -*- mode: c++ -*-
#pragma once

// Layered SIMD helpers on `<cat/simd>`. Elementwise functors and factories
// (`simd_sqrt`, `make_simd_loaded`, `simd_filled`, …) live in `<cat/simd_ops>`.
// Include that header when you need them. `<cat/simd>` does not pull it in.
//
// Chunk iterators (`as_vectorized`) live in `<cat/simd_iterator>`. Per-lane
// range iteration (P3480) lives on `<cat/simd>` (`begin` / `end`,
// `default_sentinel_t`).
//
// This header keeps lane algorithms (`simd_concat`, `simd_resize`, `simd_insert`,
// `simd_extract`, …), lane builtins (`simd_clz`, …), P3299 pointer helpers,
// `simd_chunked_invoke`, and includes x86 ABI specialization headers at the end.

namespace cat {

template <is_simd V>
[[nodiscard]]
constexpr auto
simd_concat(V const& lower_lanes_first, V const& upper_lanes_second) {
   // P2638R0 glue smaller packs into one (`concat` / split story).
   // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2638r0.pdf
   using T = typename V::scalar_type;
   using Abi = typename V::abi_type;
   using out_abi = fixed_size_abi<T, idx{Abi::lanes.raw * 2uz}>;
   simd<T, out_abi> result{};
   for (idx i = 0u; i < Abi::lanes; ++i) {
      result.set_lane(i, lower_lanes_first[i]);
      result.set_lane(i + Abi::lanes, upper_lanes_second[i]);
   }
   return result;
}

template <idx NewLanes, typename V>
   requires((is_simd<V> || is_simd_mask<V>) && NewLanes.raw != 0)
[[nodiscard]]
constexpr auto
simd_resize(V const& v) -> resize_simd<NewLanes, V> {
   // P2638R0 resize to `NewLanes` (truncate or value-init new lanes).
   // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2638r0.pdf
   resize_simd<NewLanes, V> result{};
   idx const src_lanes = V::abi_type::lanes;
   for (idx i = 0u; i < NewLanes && i < src_lanes; ++i) {
      result.set_lane(i, v[i]);
   }
   return result;
}

template <idx Begin, idx End, typename V>
   requires(is_simd_or_mask<V> && End.raw > Begin.raw
            && End.raw <= V::abi_type::lanes.raw)
[[nodiscard]]
constexpr auto
simd_extract(V const& parent)
   -> resize_simd<idx{End.raw - Begin.raw}, V> {
   // P2638R0 `extract` a contiguous lane subrange `[Begin, End)`.
   // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2638r0.pdf
   resize_simd<idx{End.raw - Begin.raw}, V> result{};
   for (idx i = Begin; i < End; ++i) {
      result.set_lane(idx(i.raw - Begin.raw), parent[i]);
   }
   return result;
}

template <idx Position, typename VParent, typename VChild>
   requires(is_simd_or_mask<VParent> && is_simd_or_mask<VChild>
            && is_same<typename VParent::scalar_type,
                       typename VChild::scalar_type>
            && Position.raw + VChild::abi_type::lanes.raw
                  <= VParent::abi_type::lanes.raw)
[[nodiscard]]
constexpr auto
simd_insert(VParent const& parent, VChild const& child) -> VParent {
   // P2638R0 `insert` a smaller pack into a lane range of the parent.
   // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2638r0.pdf
   VParent result = parent;
   for (idx i = 0u; i < VChild::abi_type::lanes; ++i) {
      result.set_lane(Position + i, child[i]);
   }
   return result;
}

template <is_simd V>
[[nodiscard]]
constexpr auto
simd_interleave(V const& from_even_lane_index, V const& from_odd_lane_index) {
   using T = typename V::scalar_type;
   using Abi = typename V::abi_type;
   using out_abi = fixed_size_abi<T, idx{Abi::lanes.raw * 2uz}>;
   simd<T, out_abi> result{};
   for (idx i = 0u; i < Abi::lanes; ++i) {
      result.set_lane(idx(i.raw * 2uz), from_even_lane_index[i]);
      result.set_lane(idx(i.raw * 2uz + 1uz), from_odd_lane_index[i]);
   }
   return result;
}

template <is_simd V>
[[nodiscard]]
constexpr auto
simd_duplicate_even(V input) -> V {
   V result{};
   for (idx i = 0u; i < V::abi_type::lanes; ++i) {
      idx const from = idx(i.raw & ~static_cast<idx::raw_type>(1u));
      result.set_lane(i, input[from]);
   }
   return result;
}

template <is_simd V>
[[nodiscard]]
constexpr auto
simd_duplicate_odd(V input) -> V {
   V result{};
   for (idx i = 0u; i < V::abi_type::lanes; ++i) {
      idx::raw_type raw = i.raw | static_cast<idx::raw_type>(1u);
      if (raw >= V::abi_type::lanes.raw) {
         raw = V::abi_type::lanes.raw - 1u;
      }
      result.set_lane(i, input[idx(raw)]);
   }
   return result;
}

template <is_simd V>
[[nodiscard]]
constexpr auto
simd_reverse_blocks(V input, idx block_lanes) -> V {
   using Abi = typename V::abi_type;
   if (block_lanes.raw == 0u || (Abi::lanes.raw % block_lanes.raw) != 0u) {
      return input;
   }
   V result{};
   for (idx base = 0u; base < Abi::lanes; base += block_lanes) {
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
template <is_simd V>
constexpr auto
simd_partial_load(V& v, typename V::memory_lane const* p, idx n) -> V& {
   return v.partial_load(p, n);
}

template <is_simd V>
constexpr void
simd_partial_store(V const& v, typename V::memory_lane* p, idx n) {
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
simd_chunked_invoke(Fn&& fn, simd<T, Abi> const& pack, Args&&... rest) {
   fwd(fn)(pack, fwd(rest)...);
}

}  // namespace cat

// x86 helpers ship with `<cat/simd>` (`simd_sse2_movmsk.hpp`,
// `simd_avx2_mask_ops.hpp`, …). SSE4.2 string helpers live in `simd_sse42.hpp`.
#include <cat/detail/simd_sse42.hpp>
