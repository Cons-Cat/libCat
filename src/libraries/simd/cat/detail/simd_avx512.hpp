#pragma once

namespace cat {

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

template <typename T>
struct avx512_abi {
   using scalar_type = T;

   template <typename U>
   using make_abi_type = avx512_abi<U>;

   constexpr avx512_abi() = delete;

   static constexpr cat::idx size = 64u;
   static constexpr cat::idx lanes{size.raw / sizeof(T)};

   template <typename ElementT>
   using simd_mask_lane =
      cat::simd_abi::mask_lane<avx512_abi<ElementT>, ElementT>;
};

template <typename T>
using avx512_simd = cat::simd<T, avx512_abi<T>>;

template <typename T>
using avx512_simd_mask = cat::simd_mask<T, avx512_abi<T>>;

namespace detail {
template <typename Abi, typename T>
inline constexpr bool is_avx512_abi_impl = false;

template <typename T>
inline constexpr bool is_avx512_abi_impl<avx512_abi<T>, T> = true;

}  // namespace detail

template <typename Abi, typename T>
concept is_avx512_abi = detail::is_avx512_abi_impl<Abi, T>;

}  // namespace x64
