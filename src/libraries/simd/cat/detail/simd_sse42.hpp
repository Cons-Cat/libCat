#pragma once

#include "cat/detail/simd_sse2.hpp"

namespace x64 {

enum class [[clang::enum_extensibility(
   closed
)]] string_control : unsigned char {
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
   return static_cast<string_control>(
      static_cast<unsigned char>(flag_1) | static_cast<unsigned char>(flag_2)
   );
}

// SSE4.2 `pcmpistri`/`pcmpistric`. `control_mask` must be a constant suitable
// for the intrinsics.
template <string_control control_mask, typename T, is_sse_abi<T> Abi>
[[gnu::target("sse4.2"), gnu::no_sanitize_address]]
constexpr auto
compare_implicit_length_strings(
   cat::simd<T, Abi> const& vector_1, cat::simd<T, Abi> const& vector_2
) -> bool {
   return __builtin_ia32_pcmpistric128(
      vector_1.raw, vector_2.raw, static_cast<unsigned char>(control_mask)
   );
}

template <string_control control_mask, typename T, is_sse_abi<T> Abi>
[[gnu::target("sse4.2"), gnu::no_sanitize_address]]
constexpr auto
compare_implicit_length_strings_return_index(
   cat::simd<T, Abi> const& vector_1, cat::simd<T, Abi> const& vector_2
) -> cat::uint4 {
   return cat::uint4(
      static_cast<unsigned>(__builtin_ia32_pcmpistri128(
         vector_1.raw, vector_2.raw, static_cast<unsigned char>(control_mask)
      ))
   );
}

}  // namespace x64
