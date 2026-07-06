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

#include "cat/simd"

namespace cat {

template <typename... Types>
struct tuple;

template <typename T, idx length>
class array;

namespace detail {

template <idx... split_sizes>
inline constexpr idx simd_split_lane_sum = idx{(split_sizes.raw + ... + 0uz)};

template <typename SimdOrMask>
struct simd_split_element;

template <typename T, typename Abi>
struct simd_split_element<simd<T, Abi>> {
   using type = T;
};

template <typename T, typename Abi>
struct simd_split_element<simd_mask<T, Abi>> {
   using type = T;
};

template <typename SimdOrMask>
using simd_split_element_type = simd_split_element<SimdOrMask>::type;

template <typename Split, typename Simd>
concept can_split_to =
   ((is_simd<Split> && is_simd<Simd>)
    || (is_simd_mask<Split> && is_simd_mask<Simd>))
   && is_same<simd_split_element_type<Split>, simd_split_element_type<Simd>>
   && Split::abi_type::lanes != 0u
   && (Simd::abi_type::lanes % Split::abi_type::lanes) == 0u;

template <idx selected_index, idx... split_sizes>
consteval auto
simd_split_offset_at() -> idx {
   idx const sizes[] = {split_sizes...};
   idx offset = 0u;
   for (idx index = 0u; index < selected_index; ++index) {
      offset += sizes[index.raw];
   }
   return offset;
}

template <idx... split_sizes, typename Simd, idx... indices>
[[nodiscard]]
constexpr auto
simd_split_impl(
   Simd const& input, [[maybe_unused]] index_list_type<indices...> /*unused*/
) {
   return tuple<resize_simd<split_sizes, Simd>...>{simd_extract<
      simd_split_offset_at<indices, split_sizes...>(),
      simd_split_offset_at<indices, split_sizes...>()
         + split_sizes...[indices.raw]>(input)...};
}

template <typename Split, typename Simd, idx... indices>
[[nodiscard]]
constexpr auto
simd_split_uniform_impl(
   Simd const& input, [[maybe_unused]] index_list_type<indices...> /*unused*/
) -> array<Split, idx{Simd::abi_type::lanes.raw / Split::abi_type::lanes.raw}> {
   constexpr idx split_lanes = Split::abi_type::lanes;
   return {
      simd_extract<
         idx{indices.raw * split_lanes.raw},
         idx{(indices.raw + 1uz) * split_lanes.raw}>(input)...,
   };
}

}  // namespace detail

template <is_simd Simd>
[[nodiscard]]
constexpr auto
simd_concat(Simd const& lower_lanes_first, Simd const& upper_lanes_second) {
   // P2638R0 glue smaller packs into one (`concat` / split story).
   // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2638r0.pdf
   using T = Simd::scalar_type;
   using abi = Simd::abi_type;
   using out_abi = simd_abi::deduce<T, idx{abi::lanes.raw * 2uz}>;
   simd<T, out_abi> result{};
   for (idx i = 0u; i < abi::lanes; ++i) {
      result.set_lane(i, lower_lanes_first[i]);
      result.set_lane(i + abi::lanes, upper_lanes_second[i]);
   }
   return result;
}

template <idx... split_sizes, is_simd_or_mask Simd>
// The split sizes must be one or more numbers which sum to the width of `Simd`.
   requires(
      sizeof...(split_sizes) > 0 && ((split_sizes != 0u) && ...)
      && detail::simd_split_lane_sum<split_sizes...> == Simd::abi_type::lanes
   )
[[nodiscard]]
constexpr auto
simd_split(Simd const& input) {
   // P2638R0 `split` turns one pack into a tuple of smaller packs.
   // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2638r0.pdf
   return detail::simd_split_impl<split_sizes...>(
      input, make_index_sequence<sizeof...(split_sizes)>{}
   );
}

template <is_simd_or_mask Split, is_simd_or_mask Simd>
   requires(detail::can_split_to<Split, Simd>)
[[nodiscard]]
constexpr auto
simd_split(
   Simd const& input
) -> array<Split, idx{Simd::abi_type::lanes.raw / Split::abi_type::lanes.raw}> {
   // P2638R0 `split<V>` splits into uniform chunks of type `V`.
   // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2638r0.pdf
   constexpr idx split_count =
      idx{Simd::abi_type::lanes.raw / Split::abi_type::lanes.raw};
   return detail::simd_split_uniform_impl<Split>(
      input, make_index_sequence<split_count>{}
   );
}

template <idx count, is_simd_or_mask Simd>
   requires(count != 0u && (Simd::abi_type::lanes % count) == 0u)
[[nodiscard]]
constexpr auto
simd_split_by(Simd const& input) {
   // P2638R0 `split_by<N>` splits one pack into `N` equal chunks.
   // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2638r0.pdf
   using split = resize_simd<idx{Simd::abi_type::lanes.raw / count.raw}, Simd>;
   return simd_split<split>(input);
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
   requires(
      is_simd_or_mask<Simd> && end_index > begin_index
      && end_index <= Simd::abi_type::lanes
   )
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
   requires(
      is_simd_or_mask<SimdParent> && is_simd_or_mask<SimdChild>
      && is_same<
         typename SimdParent::scalar_type, typename SimdChild::scalar_type>
      && position.raw + SimdChild::abi_type::lanes.raw
            <= SimdParent::abi_type::lanes.raw
   )
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
simd_interleave(
   Simd const& from_even_lane_index, Simd const& from_odd_lane_index
) {
   using T = Simd::scalar_type;
   using abi = Simd::abi_type;
   using out_abi = simd_abi::deduce<T, idx{abi::lanes.raw * 2uz}>;
   simd<T, out_abi> result{};
   for (idx i = 0u; i < abi::lanes; ++i) {
      result.set_lane(idx(i.raw * 2uz), from_even_lane_index[i]);
      result.set_lane(idx((i.raw * 2uz) + 1uz), from_odd_lane_index[i]);
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
   using abi = Simd::abi_type;
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
simd_partial_load(
   Simd& v, typename Simd::memory_lane const* _Nonnull p_source, idx n
) -> Simd& {
   return v.partial_load(p_source, n);
}

template <is_simd Simd>
constexpr void
simd_partial_store(
   Simd const& v, typename Simd::memory_lane* _Nonnull p_destination, idx n
) {
   v.partial_store(p_destination, n);
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

// x86 helpers ship with `<cat/simd>` (`simd_sse_movmsk.hpp`,
// `simd_avx2_mask_ops.hpp`, ...). SSE4.2 string helpers live in
// `simd_sse42.hpp`.

#include <cat/detail/simd_sse42.hpp>
