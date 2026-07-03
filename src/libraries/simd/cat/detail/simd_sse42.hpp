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

// 16-byte XMM layout (`movaps`-aligned). Used for baseline 16-byte SIMD and
// for code paths that call SSE4.2 string intrinsics at runtime while.
template <typename T>
struct sse_abi {
   using scalar_type = T;

   template <typename U>
   using make_abi_type = sse_abi<U>;

   constexpr sse_abi() = delete;

   static constexpr cat::idx size = 16u;
   static constexpr cat::idx lanes{size.raw / sizeof(T)};
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

enum class string_control : unsigned char {
   unsigned_byte = 0x00,
   unsigned_word = 0x01,
   signed_byte = 0x02,
   signed_word = 0x03,
   compare_equal_any = 0x00,
   compare_ranges = 0x04,
   compare_equal_each = 0x08,
   compare_equal_ordered = 0x0c,
   positive_polarity = 0x00,
   negative_polarity = 0x10,
   masked_positive_polarity = 0x20,
   masked_negative_polarity = 0x30,
   least_significant = 0x00,
   most_significant = 0x40,
   bit_mask = 0x00,
   unit_mask = 0x40,
};

// TODO: Generalize this.
constexpr auto
operator|(string_control flag_1, string_control flag_2) -> string_control {
   return static_cast<string_control>(static_cast<unsigned char>(flag_1)
                                      | static_cast<unsigned char>(flag_2));
}

}  // namespace x64

#include <cat/simd>

namespace x64 {

// SSE4.2 `pcmpistri`/`pcmpistric`. `control_mask` must be a constant suitable
// for the intrinsics.
template <string_control control_mask, typename T, is_sse_abi<T> Abi>
[[gnu::target("sse4.2"), gnu::no_sanitize_address]]
constexpr auto
compare_implicit_length_strings(cat::simd<T, Abi> const& vector_1,
                                cat::simd<T, Abi> const& vector_2) -> bool {
   return __builtin_ia32_pcmpistric128(
      vector_1.raw, vector_2.raw, static_cast<unsigned char>(control_mask));
}

template <string_control control_mask, typename T, is_sse_abi<T> Abi>
[[gnu::target("sse4.2"), gnu::no_sanitize_address]]
constexpr auto
compare_implicit_length_strings_return_index(cat::simd<T, Abi> const& vector_1,
                                             cat::simd<T, Abi> const& vector_2)
   -> cat::uint4 {
   return cat::uint4(static_cast<unsigned>(__builtin_ia32_pcmpistri128(
      vector_1.raw, vector_2.raw, static_cast<unsigned char>(control_mask))));
}

}  // namespace x64
