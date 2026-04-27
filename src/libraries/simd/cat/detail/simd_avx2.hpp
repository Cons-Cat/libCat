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

// `avx2_abi` is a SIMD ABI that can be expected to work on most reasonable
// x86-64 build target.
template <typename T>
struct avx2_abi {
   using scalar_type = T;

   template <typename U>
   using make_abi_type = avx2_abi<U>;

   constexpr avx2_abi() = delete;

   static constexpr cat::idx size = 32u;
   static constexpr cat::idx lanes{size.raw / sizeof(T)};
   static constexpr cat::uword alignment = 32u;

   // Physical storage and lane truth encoding for `simd_mask` (see
   // `simd_abi::mask_lane` and `simd_mask_lane_sseavx.hpp`).
   template <typename ElementT>
   using simd_mask_lane =
      cat::simd_abi::mask_lane<avx2_abi<ElementT>, ElementT>;
};

template <typename T>
using avx2_simd = cat::simd<T, avx2_abi<T>>;

template <typename T>
using avx2_simd_mask = cat::simd_mask<T, avx2_abi<T>>;

template <typename T>
using avx2_unaligned_abi = cat::unaligned_abi<avx2_abi<T>>;

template <typename T>
using avx2_unaligned_simd = cat::simd<T, avx2_unaligned_abi<T>>;

template <typename T>
using avx2_unaligned_simd_mask = cat::simd_mask<T, avx2_unaligned_abi<T>>;

}  // namespace x64

namespace x64 {

// Store fence. Use after non-temporal (`movnt*`) stores so later loads see
// them.
[[gnu::always_inline]]
inline void
sfence() {
   __builtin_ia32_sfence();
}

// Full memory fence. Orders prior stores and loads against later stores and
// loads.
[[gnu::always_inline]]
inline void
mfence() {
   __builtin_ia32_mfence();
}

// Load fence. Orders prior loads against later loads (and with `lfence/mfence`
// pairs).
[[gnu::always_inline]]
inline void
lfence() {
   __builtin_ia32_lfence();
}

// `vzeroall`. Zeros all ymm registers (and zmm when applicable).
[[gnu::always_inline]]
inline void
zero_avx_registers() {
   __builtin_ia32_vzeroall();
}

// `vzeroupper`. Zeros the upper 128 bits of each ymm register for AVX-to-SSE
// transitions.
[[gnu::always_inline]]
inline void
zero_upper_avx_registers() {
   __builtin_ia32_vzeroupper();
}

}  // namespace x64

// AVX2 and AVX2-unaligned `simd_abi::mask_lane` share the xmm and ymm packed
// lane mask encoding in `simd_mask_lane_sseavx.hpp`.
