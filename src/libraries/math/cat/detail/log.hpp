// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/ldexp.hpp>

#include <cat/math>

namespace cat::detail {

// TODO: Replace this approximation with a native libCat log kernel.
template <is_raw_floating_point Float>
[[nodiscard]]
constexpr auto
emulated_log(Float argument) -> Float {
   if (is_nan(argument)) {
      return argument;
   }
   if (argument < 0) {
      return limits<Float>::quiet_NaN();
   }
   if (argument == 0) {
      return -infinity;
   }
   if (!is_finite(argument)) {
      return argument;
   }

   using traits = ldexp_float_bits<Float>;
   using uint_type = traits::uint_type;
   constexpr uint_type fraction_mask =
      (uint_type(1) << traits::fraction_width) - uint_type(1);
   constexpr uint_type one_bits = uint_type(traits::exponent_bias)
                                  << traits::fraction_width;
   constexpr Float minimum_normal =
      is_same<Float, float> ? 0x1p-126f : 0x1p-1022;

   int exponent_adjustment = 0;
   while (argument < minimum_normal) {
      argument *= 2;
      --exponent_adjustment;
   }

   uint_type const bits = __builtin_bit_cast(uint_type, argument);
   int exponent = static_cast<int>(bits >> traits::fraction_width)
                  - traits::exponent_bias + exponent_adjustment;
   Float mantissa =
      __builtin_bit_cast(Float, (bits & fraction_mask) | one_bits);
   if (mantissa > 0x1.6a09e667f3bcdp+0) {
      mantissa *= 0.5;
      ++exponent;
   }

   Float const ratio = (mantissa - 1) / (mantissa + 1);
   Float const squared = ratio * ratio;
   Float term = ratio;
   Float sum = ratio;
   constexpr idx term_count = is_same<Float, float> ? 8u : 20u;
   for (idx index = 1u; index < term_count; ++index) {
      term *= squared;
      sum += term / Float(make_raw_arithmetic(index * 2u + 1u));
   }

   constexpr Float ln_two =
      is_same<Float, float> ? 0x1.62e43p-1f : 0x1.62e42fefa39efp-1;
   return 2 * sum + exponent * ln_two;
}

}  // namespace cat::detail

namespace cat {

template <is_floating_point Float>
[[nodiscard]]
constexpr auto
log(Float argument) -> Float {
   if consteval {
      return Float(detail::emulated_log(make_raw_arithmetic(argument)));
   }
   return Float(__builtin_elementwise_log(make_raw_arithmetic(argument)));
}

}  // namespace cat
