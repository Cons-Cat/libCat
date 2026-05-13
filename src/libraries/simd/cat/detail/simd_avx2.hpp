#pragma once

#include <cat/detail/simd_unaligned_abi.hpp>

namespace cat {

// Forward declarations. Element type parameter before ABI tag.
template <typename T, typename Abi>
   requires(is_same<typename Abi::scalar_type, T>)
class alignas(Abi::alignment.raw) simd;

template <typename T, typename Abi>
class alignas(Abi::alignment.raw) simd_mask;

namespace simd_abi {
template <typename AbiTag, typename ElementT>
struct mask_lane;
}

}  // namespace cat

namespace x64 {

// `avx_abi` is a 32-byte (ymm) SIMD ABI. The vector layout itself only
// requires AVX. AVX2-locked operations (e.g. `pmovmskb256`) live in
// per-arch hook headers (`simd_avx2_mask_ops.hpp`) and pin their own
// `gnu::target("avx2")` rather than promoting the whole ABI to AVX2.
template <typename T>
struct avx_abi {
   using scalar_type = T;

   template <typename U>
   using make_abi_type = avx_abi<U>;

   constexpr avx_abi() = delete;

   static constexpr cat::idx size = 32u;
   static constexpr cat::idx lanes{size.raw / sizeof(T)};
   static constexpr cat::uword alignment = 32u;

   // Physical storage and lane truth encoding for `simd_mask` (see
   // `simd_abi::mask_lane` and `simd_mask_lane_sseavx.hpp`).
   template <typename ElementT>
   using simd_mask_lane = cat::simd_abi::mask_lane<avx_abi<ElementT>, ElementT>;
};

template <typename T>
using avx_simd = cat::simd<T, avx_abi<T>>;

template <typename T>
using avx_simd_mask = cat::simd_mask<T, avx_abi<T>>;

template <typename T>
using avx_unaligned_abi = cat::simd_abi::unaligned<avx_abi<T>>;

template <typename T>
using avx_unaligned_simd = cat::simd<T, avx_unaligned_abi<T>>;

template <typename T>
using avx_unaligned_simd_mask = cat::simd_mask<T, avx_unaligned_abi<T>>;

namespace detail {
template <typename Abi, typename T>
inline constexpr bool is_avx_abi_impl = false;

template <typename T>
inline constexpr bool is_avx_abi_impl<avx_abi<T>, T> = true;

template <typename T>
inline constexpr bool is_avx_abi_impl<avx_unaligned_abi<T>, T> = true;
}  // namespace detail

// Matches both `avx_abi<T>` and unaligned `avx_unaligned_abi<T>`.
template <typename Abi, typename T>
concept is_avx_abi = detail::is_avx_abi_impl<Abi, T>;

}  // namespace x64

// `cat::detail::simd_avx2`'s `native_simd<T>` / `deduce_simd<T, lanes>`
// / etc. aliases are populated via `CAT_SIMD_ALIASES(::x64::avx_abi)`
// in `<cat/detail/simd_aliases.hpp>`, after the `cat::simd` template is
// fully declared. `$simd_switch($abi(avx2, ...))` injects this namespace into
// the body via `using namespace`, so `native_simd<T>` etc. resolve to
// the AVX2-pinned vector for that arch lane.

namespace x64 {
// Store fence. Use after non-temporal (`movnt*`) stores so later loads see
// them.
[[gnu::target("sse"), gnu::always_inline]]
inline void
sfence() {
   __builtin_ia32_sfence();
}

// Full memory fence. Orders prior stores and loads against later stores and
// loads.
[[gnu::target("sse2"), gnu::always_inline]]
inline void
mfence() {
   __builtin_ia32_mfence();
}

// Load fence. Orders prior loads against later loads (and with
// `lfence/mfence` pairs).
[[gnu::target("sse2"), gnu::always_inline]]
inline void
lfence() {
   __builtin_ia32_lfence();
}

// `vzeroall`. Zeros all ymm registers (and zmm when applicable).
[[gnu::target("avx")]]
inline void
zero_avx_registers() {
   __builtin_ia32_vzeroall();
}

// `vzeroupper`. Zeros the upper 128 bits of each ymm register for AVX-to-SSE
// transitions.
[[gnu::target("avx")]]
inline void
zero_upper_avx_registers() {
   __builtin_ia32_vzeroupper();
}

}  // namespace x64

// AVX2 and AVX2-unaligned `simd_abi::mask_lane` share the xmm and ymm packed
// lane mask encoding in `simd_mask_lane_sseavx.hpp`.
