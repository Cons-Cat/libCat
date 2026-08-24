#pragma once

// `cat::detail::simd_avx2`'s `native_simd<T>` / `deduce_simd<T, lanes>`
// / etc. aliases are populated via `CAT_SIMD_ALIASES(::x64::avx_abi)`
// in `<cat/detail/simd_aliases.hpp>`, after the `cat::simd` template is
// fully declared. `$simd_switch($abi(avx2, ...))` injects this namespace into
// the body via `using namespace`, so `native_simd<T>` etc. resolve to
// the AVX2-pinned vector for that arch lane.

#include "cat/detail/simd_avx.hpp"

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
