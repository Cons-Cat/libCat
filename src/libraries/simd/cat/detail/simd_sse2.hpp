#pragma once

#include <cat/detail/simd_unaligned_abi.hpp>

#include <cat/arithmetic>
#include <cat/meta>

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

// 16-byte layout.
template <typename T>
struct sse_abi {
   using scalar_type = T;

   template <typename U>
   using make_abi_type = sse_abi<U>;

   constexpr sse_abi() = delete;

   static constexpr cat::idx size = 16u;
   static constexpr cat::idx lanes{size / sizeof(T)};
   static constexpr cat::ualign alignment = 16u;

   template <typename ElementT>
   using simd_mask_lane = cat::simd_abi::mask_lane<sse_abi<ElementT>, ElementT>;
};

template <typename T>
using sse_simd = cat::simd<T, sse_abi<T>>;

template <typename T>
using sse_simd_mask = cat::simd_mask<T, sse_abi<T>>;

template <typename T>
using sse_unaligned_abi = cat::simd_abi::unaligned<sse_abi<T>>;

template <typename T>
using sse_unaligned_simd = cat::simd<T, sse_unaligned_abi<T>>;

template <typename T>
using sse_unaligned_simd_mask = cat::simd_mask<T, sse_unaligned_abi<T>>;

namespace detail {
template <typename Abi, typename T>
inline constexpr bool is_sse_abi_impl = false;

template <typename T>
inline constexpr bool is_sse_abi_impl<sse_abi<T>, T> = true;

template <typename T>
inline constexpr bool is_sse_abi_impl<sse_unaligned_abi<T>, T> = true;
}  // namespace detail

// Matches both `sse_abi<T>` and unaligned `sse_unaligned_abi<T>`.
template <typename Abi, typename T>
concept is_sse_abi = detail::is_sse_abi_impl<Abi, T>;

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

}  // namespace x64
