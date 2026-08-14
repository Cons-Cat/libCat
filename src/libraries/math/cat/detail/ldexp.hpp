// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>

namespace cat::detail {

template <is_raw_floating_point Float>
struct ldexp_float_bits;

template <>
struct ldexp_float_bits<float> {
   using uint_type = __UINT32_TYPE__;

   static constexpr int fraction_width = 23;
   static constexpr int exponent_bias = 127;
   static constexpr float max_scale = 0x1p127f;
   static constexpr float min_scale = 0x1p-126f;
};

template <>
struct ldexp_float_bits<double> {
   using uint_type = __UINT64_TYPE__;

   static constexpr int fraction_width = 52;
   static constexpr int exponent_bias = 1'023;
   static constexpr double max_scale = 0x1p1023;
   static constexpr double min_scale = 0x1p-1022;
};

template <is_raw_floating_point Float>
[[nodiscard]]
constexpr auto
emulated_ldexp(Float value, int exponent) -> Float {
   using traits = ldexp_float_bits<Float>;
   constexpr int max_exponent = traits::exponent_bias;
   constexpr int min_exponent = 1 - max_exponent;
   Float result = value;

   if (exponent > max_exponent) {
      result *= traits::max_scale;
      exponent -= max_exponent;
      if (exponent > max_exponent) {
         result *= traits::max_scale;
         exponent -= max_exponent;
         exponent = exponent > max_exponent ? max_exponent : exponent;
      }
   } else if (exponent < min_exponent) {
      result *= traits::min_scale;
      exponent -= min_exponent;
      if (exponent < min_exponent) {
         result *= traits::min_scale;
         exponent -= min_exponent;
         exponent = exponent < min_exponent ? min_exponent : exponent;
      }
   }

   using uint_type = traits::uint_type;
   uint_type const scale_bits = uint_type(exponent + max_exponent)
                                << traits::fraction_width;
   return result * __builtin_bit_cast(Float, scale_bits);
}

}  // namespace cat::detail
