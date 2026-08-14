// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/ldexp.hpp>

#include <cat/math>

namespace cat::detail {

// TODO: Replace this approximation with a native libCat exp kernel.
template <is_raw_floating_point Float>
[[nodiscard]]
constexpr auto
emulated_exp(Float argument) -> Float {
   if (is_nan(argument)) {
      return argument;
   }
   constexpr Float overflow =
      is_same<Float, float> ? 88.722839f : 709.78271289338397;
   constexpr Float underflow =
      is_same<Float, float> ? -103.972084f : -745.1332191019411;
   if (argument > overflow) {
      return infinity;
   }
   if (argument < underflow) {
      return 0;
   }

   constexpr Float inverse_ln_two =
      is_same<Float, float> ? 0x1.715476p+0f : 0x1.71547652b82fep+0;
   constexpr Float ln_two_hi =
      is_same<Float, float> ? 0x1.62e4p-1f : 0x1.62e42fefa0000p-1;
   constexpr Float ln_two_lo =
      is_same<Float, float> ? 0x1.7f7d1cp-20f : 0x1.cf79abc9e3b3ap-40;
   Float const scaled = argument * inverse_ln_two;
   int const exponent = static_cast<int>(scaled + (scaled < 0 ? -0.5 : 0.5));
   Float const exponent_float = exponent;
   Float const reduced =
      (argument - exponent_float * ln_two_hi) - exponent_float * ln_two_lo;

   Float term = 1;
   Float sum = 1;
   constexpr idx term_count = is_same<Float, float> ? 12u : 20u;
   for (idx index = 1u; index <= term_count; ++index) {
      term *= reduced / Float(make_raw_arithmetic(index));
      sum += term;
   }
   return emulated_ldexp(sum, exponent);
}

}  // namespace cat::detail

namespace cat {

template <is_floating_point Float>
[[nodiscard]]
constexpr auto
exp(Float argument) -> Float {
   if consteval {
      return Float(detail::emulated_exp(make_raw_arithmetic(argument)));
   }
   return Float(__builtin_elementwise_exp(make_raw_arithmetic(argument)));
}

}  // namespace cat
