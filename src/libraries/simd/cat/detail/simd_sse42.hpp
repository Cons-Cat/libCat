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

// 16-byte XMM layout (`movaps`-aligned). Used for baseline 128-bit SIMD and for
// code paths that call SSE4.2 string intrinsics at runtime while still typing
// vectors as this ABI.
template <typename T>
struct sse2_abi {
   using scalar_type = T;

   template <typename U>
   using make_abi_type = sse2_abi<U>;

   constexpr sse2_abi() = delete;

   static constexpr cat::idx size = 16u;
   static constexpr cat::idx lanes{size.raw / sizeof(T)};
   static constexpr cat::uword alignment = 16u;

   template <typename ElementT>
   using simd_mask_lane =
      cat::simd_abi::mask_lane<sse2_abi<ElementT>, ElementT>;
};

template <typename T>
using sse2_simd = cat::simd<T, sse2_abi<T>>;

template <typename T>
using sse2_simd_mask = cat::simd_mask<T, sse2_abi<T>>;

template <typename T>
using sse2_unaligned_abi = cat::unaligned_abi<sse2_abi<T>>;

template <typename T>
using sse2_unaligned_simd = cat::simd<T, sse2_unaligned_abi<T>>;

template <typename T>
using sse2_unaligned_simd_mask = cat::simd_mask<T, sse2_unaligned_abi<T>>;

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

// SSE4.2 `pcmpistri`/`pcmpistric` on `simd` with `sse2_abi` layout (128-bit XMM
// `raw`). `control_mask` must be a constant suitable for the intrinsics.
template <string_control control_mask, typename T>
[[gnu::no_sanitize_address]]
constexpr auto
compare_implicit_length_strings(cat::simd<T, sse2_abi<T>> const& vector_1,
                                cat::simd<T, sse2_abi<T>> const& vector_2)
   -> bool {
   return __builtin_ia32_pcmpistric128(
      vector_1.raw, vector_2.raw, static_cast<unsigned char>(control_mask));
}

template <string_control control_mask, typename T>
[[gnu::no_sanitize_address]]
constexpr auto
compare_implicit_length_strings(
   cat::simd<T, sse2_unaligned_abi<T>> const& vector_1,
   cat::simd<T, sse2_unaligned_abi<T>> const& vector_2) -> bool {
   return __builtin_ia32_pcmpistric128(
      vector_1.raw, vector_2.raw, static_cast<unsigned char>(control_mask));
}

template <string_control control_mask, typename T>
[[gnu::no_sanitize_address]]
constexpr auto
compare_implicit_length_strings_return_index(
   cat::simd<T, sse2_abi<T>> const& vector_1,
   cat::simd<T, sse2_abi<T>> const& vector_2) -> cat::uint4 {
   return cat::uint4(static_cast<unsigned>(__builtin_ia32_pcmpistri128(
      vector_1.raw, vector_2.raw, static_cast<unsigned char>(control_mask))));
}

template <string_control control_mask, typename T>
[[gnu::no_sanitize_address]]
constexpr auto
compare_implicit_length_strings_return_index(
   cat::simd<T, sse2_unaligned_abi<T>> const& vector_1,
   cat::simd<T, sse2_unaligned_abi<T>> const& vector_2) -> cat::uint4 {
   return cat::uint4(static_cast<unsigned>(__builtin_ia32_pcmpistri128(
      vector_1.raw, vector_2.raw, static_cast<unsigned char>(control_mask))));
}

}  // namespace x64
