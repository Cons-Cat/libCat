// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/ldexp.hpp>

#include <cat/arithmetic>

namespace cat::detail {

// Ported from LLVM libc's `fputil::floor()` and `fputil::ceil()`.
template <is_raw_floating_point Float>
[[nodiscard]]
constexpr auto
emulated_trunc(Float argument) noexcept -> Float {
   using traits = ldexp_float_bits<Float>;
   using uint_type = traits::uint_type;
   constexpr uint_type sign_mask = uint_type(1) << (sizeof(uint_type) * 8 - 1);

   if (!is_finite(argument)) {
      return argument;
   }
   uint_type bits = __builtin_bit_cast(uint_type, argument);
   int const exponent =
      static_cast<int>((bits & ~sign_mask) >> traits::fraction_width)
      - traits::exponent_bias;
   // `argument` is already an integer.
   if (exponent >= traits::fraction_width) {
      return argument;
   }
   // The magnitude of `argument` is less than 1.
   if (exponent <= -1) {
      return __builtin_bit_cast(Float, bits & sign_mask);
   }
   int const trim_size = traits::fraction_width - exponent;
   bits = (bits >> trim_size) << trim_size;
   return __builtin_bit_cast(Float, bits);
}

template <is_raw_floating_point Float>
[[nodiscard]]
constexpr auto
emulated_ceil(Float argument) noexcept -> Float {
   Float const truncated = emulated_trunc(argument);
   // Truncating a negative value, or an integer, already rounds it up.
   if (truncated == argument || argument < 0) {
      return truncated;
   }
   return truncated + 1;
}

template <is_raw_floating_point Float>
[[nodiscard]]
constexpr auto
emulated_floor(Float argument) noexcept -> Float {
   if (argument < 0) {
      return -emulated_ceil(-argument);
   }
   return emulated_trunc(argument);
}

}  // namespace cat::detail

namespace cat {

template <is_integral T>
[[nodiscard]]
constexpr auto
floor(T value) noexcept -> T {
   return value;
}

template <is_floating_point Float>
[[nodiscard]]
constexpr auto
floor(Float value) noexcept -> Float {
   if consteval {
      return Float(detail::emulated_floor(make_raw_arithmetic(value)));
   }
   return Float(__builtin_elementwise_floor(make_raw_arithmetic(value)));
}

template <is_integral T>
[[nodiscard]]
constexpr auto
ceil(T value) noexcept -> T {
   return value;
}

template <is_floating_point Float>
[[nodiscard]]
constexpr auto
ceil(Float value) noexcept -> Float {
   if consteval {
      return Float(detail::emulated_ceil(make_raw_arithmetic(value)));
   }
   return Float(__builtin_elementwise_ceil(make_raw_arithmetic(value)));
}

}  // namespace cat
