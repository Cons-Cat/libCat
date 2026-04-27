// -*- mode: c++ -*-
#pragma once

#include <cat/meta>
#include <cat/utility>

// Optional ABI-specific full-vector implementations for `simd_*` helpers beyond
// lane arithmetic and load or store. Specialize these templates in a target
// header after `<cat/simd>`. Each specialization provides `static constexpr`
// `invoke` with the signature expected by `simd_extras.hpp`.
//
// Call sites use `if constexpr (requires { unary_full<...>::invoke(...) })` and
// `invoke<Expected, Hook, ...>(...)`. Mismatched return types fail
// `static_assert` with an explicit message instead of a terse concept error.
//
// `mask_to_bitset<T, Abi>` specializations for x86 ABIs live in
// `implementations/simd_mask_bitset.tpp` beside `simd_to_bitset` (after
// `<cat/bitset>`). Each `invoke(simd_mask<...>)` returns an unsigned lane-bit
// pattern (movmsk-style encoding), not `cat::bitset`, so the bitset header does
// not create a cycle. `simd_to_bitset` passes `Expected` =
// `decltype(mask_to_bitset<T, Abi>::invoke(declval<simd_mask<T, Abi>>()))` into
// `invoke<Expected, ...>`.
//
// Operation tags name optional ISA fast paths. When `invoke` exists,
// `simd_extras.hpp` prefers it over `__builtin_elementwise_*`, which do not
// model per-lane masking. Masked `simd_*[lane_mask]` uses `invoke_masked` and
// optional `binary_masked` or `ternary_masked`. Otherwise the library uses
// per-lane fallbacks.
//
// Primary templates `unary_full`, `binary_full`, `ternary_full`,
// `ternary_masked`, and `binary_mixed` have no `invoke` until an ISA header
// adds one, so the elementwise path stays available. `simd::popcount()` remains
// the member and elementwise builtin story. There is no elementwise full-vector
// rotate builtin. `op_rsqrt` pairs with `simd_rsqrt` (see `simd_avx2.hpp` and
// `simd_sse2.hpp`). `op_rotate_*` pairs with `unary_int` overrides or lane
// loops in `simd_extras.hpp`.
//
// `binary_masked` and `has_simd_abi_binary_masked` cover blends with packed ops
// when `invoke_full` does not encode inactive lanes and the target wants an
// instruction plus blend. Unspecialized `binary_masked` has no `invoke`.
// Functors that pass an ABI op tag through `simd_elementwise_binary_interface`
// blend `invoke_full` with `left` on inactive lanes and keep `invoke_lanewise`
// for explicit per-lane use (`simd_extras.hpp`).
//
// `ternary_masked` and `has_simd_abi_ternary_masked` cover masked three-operand
// paths (for example AVX-512 masked FMA). Unspecialized `ternary_masked` has no
// `invoke` until an ISA header defines one (`simd_extras.hpp`).
//
// `fill_masked<T, Abi>` implements lane-blended scalar fill. Broadcast into
// lanes selected by `lane_mask`, and keep `passthrough` elsewhere
// (`simd_filled<Simd>[lane_mask](passthrough, value)` in `simd_ops`).
// `simd_ops` selects it when `has_simd_abi_fill_masked<T, Abi>` holds. The
// primary template has no `invoke` until a target supplies one (for example
// AVX-512 `vblendm*` or mask registers with a splat).

namespace cat::detail::simd_abi {

template <typename Expected, typename Hook, typename... Args>
[[nodiscard]]
constexpr auto
invoke(Args&&... arguments) -> Expected {
   auto const result = Hook::invoke(static_cast<Args&&>(arguments)...);
   static_assert(
      is_same<Expected, remove_cvref<decltype(result)>>,
      "simd_abi specialization: Hook::invoke must return exactly the type "
      "Expected at this call site");
   return result;
}

struct op_sqrt {};

struct op_rsqrt {};

struct op_sin {};

struct op_cos {};

struct op_tan {};

struct op_asin {};

struct op_acos {};

struct op_atan {};

struct op_sinh {};

struct op_cosh {};

struct op_tanh {};

struct op_ceil {};

struct op_floor {};

struct op_log {};

struct op_log2 {};

struct op_log10 {};

struct op_exp {};

struct op_exp2 {};

struct op_exp10 {};

struct op_roundeven {};

struct op_round {};

struct op_trunc {};

struct op_nearbyint {};

struct op_rint {};

struct op_canonicalize {};

struct op_abs {};

struct op_bitreverse {};

struct op_pow {};

struct op_atan2 {};

struct op_copysign {};

struct op_fmod {};

struct op_minnum {};

struct op_maxnum {};

struct op_minimumnum {};

struct op_maximumnum {};

struct op_minimum {};

struct op_maximum {};

struct op_add_sat {};

struct op_sub_sat {};

struct op_fma {};

struct op_fshl {};

struct op_fshr {};

struct op_ldexp {};

struct op_rotate_left {};

struct op_rotate_right {};

template <typename OpTag, typename T, typename Abi>
struct unary_full {};

template <typename OpTag, typename T, typename Abi>
struct binary_full {};

template <typename OpTag, typename T, typename Abi>
struct binary_masked {};

template <typename OpTag, typename T, typename Abi>
struct ternary_full {};

template <typename OpTag, typename T, typename Abi>
struct ternary_masked {};

template <typename OpTag, typename T, typename IntT, typename Abi>
struct binary_mixed {};

template <typename OpTag, typename T, typename Abi>
struct unary_int {};

template <typename T, typename Abi>
struct compress {};

template <typename T, typename Abi>
struct expand {};

// Lane-blended fill. `invoke(passthrough, fill_value, lane_mask)` returns
// `simd<T, Abi>`. The primary template has no `invoke` until a target header
// defines one. `simd_ops` dispatches to a specialization when present (see file
// comment above).
template <typename T, typename Abi>
struct fill_masked {};

template <typename T, typename Abi>
struct mask_to_bitset {};

// Optional overrides for `mask_count_if_true`, `mask_find_if_true`,
// `mask_find_last_if_true`, `mask_all_of`, and `mask_any_of` expose `static`
// `invoke(...)`. Primaries with scalar lane loops live in `<cat/simd>` after
// `simd_mask`. AVX2 `movmsk` partial specializations live in
// `simd_avx2_mask_ops.hpp`.

struct op_concat {};

struct op_interleave {};

struct op_duplicate_even {};

struct op_duplicate_odd {};

struct op_reverse_blocks {};

template <typename OpTag, typename Simd>
struct widen_pair {};

template <typename OpTag, typename Simd>
struct same_width {};

template <typename OpTag, typename Simd>
struct reverse_blocks_layout {};

}  // namespace cat::detail::simd_abi
