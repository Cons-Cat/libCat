#pragma once

namespace cat {

// Forward declarations. Element type parameter before ABI tag.
template <typename T, typename Abi>
   requires(is_same<typename Abi::scalar_type, T>)
class simd;

template <typename T, typename Abi>
class simd_mask;

namespace simd_abi {
template <typename AbiTag, typename ElementT>
struct mask_lane;
}

}  // namespace cat

namespace x64 {

// 32-byte SIMD ABI.
template <typename T>
struct avx_abi {
   using scalar_type = T;

   template <typename U>
   using make_abi_type = avx_abi<U>;

   constexpr avx_abi() = delete;

   static constexpr cat::idx size = 32u;
   static constexpr cat::idx lanes{size.raw / sizeof(T)};

   // Physical storage and lane truth encoding for `simd_mask` (see
   // `simd_abi::mask_lane` and `simd_mask_lane_sseavx.hpp`).
   template <typename ElementT>
   using simd_mask_lane = cat::simd_abi::mask_lane<avx_abi<ElementT>, ElementT>;
};

template <typename T>
using avx_simd = cat::simd<T, avx_abi<T>>;

template <typename T>
using avx_simd_mask = cat::simd_mask<T, avx_abi<T>>;

namespace detail {
template <typename Abi, typename T>
inline constexpr bool is_avx_abi_impl = false;

template <typename T>
inline constexpr bool is_avx_abi_impl<avx_abi<T>, T> = true;

}  // namespace detail

template <typename Abi, typename T>
concept is_avx_abi = detail::is_avx_abi_impl<Abi, T>;

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
