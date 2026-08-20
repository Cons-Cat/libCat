// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/modulo.hpp>
#include <cat/detail/sqrt.hpp>

#include <cat/math>

// TODO: Replace these series with native libCat trigonometric kernels.
namespace cat::detail {

template <is_raw_floating_point Float>
[[nodiscard]]
constexpr auto
emulated_reduce_angle(Float argument) -> Float {
   Float reduced = fmod_integer(argument, tau<Float>);
   if (reduced > pi<Float>) {
      reduced -= tau<Float>;
   } else if (reduced < -pi<Float>) {
      reduced += tau<Float>;
   }
   return reduced;
}

template <is_raw_floating_point Float>
[[nodiscard]]
constexpr auto
emulated_sin(Float argument) -> Float {
   if (!is_finite(argument)) {
      return is_nan(argument) ? argument : limits<Float>::quiet_NaN();
   }
   Float const reduced = emulated_reduce_angle(argument);
   Float const squared = reduced * reduced;
   Float term = reduced;
   Float result = reduced;
   constexpr idx term_count = is_same<Float, float> ? 7u : 12u;
   for (idx index = 1u; index < term_count; ++index) {
      Float const even = Float(make_raw_arithmetic(index * 2u));
      term *= -squared / (even * (even + 1));
      result += term;
   }
   return result;
}

template <is_raw_floating_point Float>
[[nodiscard]]
constexpr auto
emulated_cos(Float argument) -> Float {
   if (!is_finite(argument)) {
      return is_nan(argument) ? argument : limits<Float>::quiet_NaN();
   }
   Float const reduced = emulated_reduce_angle(argument);
   Float const squared = reduced * reduced;
   Float term = 1;
   Float result = 1;
   constexpr idx term_count = is_same<Float, float> ? 8u : 13u;
   for (idx index = 1u; index < term_count; ++index) {
      Float const odd = Float(make_raw_arithmetic(index * 2u - 1u));
      term *= -squared / (odd * (odd + 1));
      result += term;
   }
   return result;
}

template <is_raw_floating_point Float>
[[nodiscard]]
constexpr auto
emulated_tan(Float argument) -> Float {
   return emulated_sin(argument) / emulated_cos(argument);
}

template <is_raw_floating_point Float>
[[nodiscard]]
constexpr auto
emulated_atan_series(Float argument) -> Float {
   Float const squared = argument * argument;
   Float term = argument;
   Float result = argument;
   constexpr idx term_count = is_same<Float, float> ? 12u : 32u;
   for (idx index = 1u; index < term_count; ++index) {
      term *= -squared;
      result += term / Float(make_raw_arithmetic(index * 2u + 1u));
   }
   return result;
}

template <is_raw_floating_point Float>
[[nodiscard]]
constexpr auto
emulated_atan(Float argument) -> Float {
   if (is_nan(argument)) {
      return argument;
   }

   bool const negative = __builtin_signbit(argument) != 0;
   Float const magnitude = negative ? -argument : argument;
   Float result;
   if (!is_finite(magnitude)) {
      result = pi<Float> / 2;
   } else if (magnitude > 1) {
      result = pi<Float> / 2 - emulated_atan(1 / magnitude);
   } else {
      constexpr Float tan_pi_over_eight =
         is_same<Float, float> ? 0x1.a8279ap-2f : 0x1.a827999fcef32p-2;
      result = magnitude > tan_pi_over_eight
                  ? pi<Float> / 4
                       + emulated_atan_series((magnitude - 1) / (magnitude + 1))
                  : emulated_atan_series(magnitude);
   }
   return negative ? -result : result;
}

template <is_raw_floating_point Float>
[[nodiscard]]
constexpr auto
emulated_atan2(Float y, Float x) -> Float {
   if (is_nan(x) || is_nan(y)) {
      return x + y;
   }

   bool const y_negative = __builtin_signbit(y) != 0;
   bool const x_negative = __builtin_signbit(x) != 0;
   if (y == 0) {
      if (x_negative) {
         return y_negative ? -pi<Float> : pi<Float>;
      }
      return y;
   }
   if (x == 0) {
      return y_negative ? -pi<Float> / 2 : pi<Float> / 2;
   }

   bool const y_finite = is_finite(y);
   bool const x_finite = is_finite(x);
   if (!y_finite && !x_finite) {
      Float const angle = x_negative ? pi<Float> * 3 / 4 : pi<Float> / 4;
      return y_negative ? -angle : angle;
   }
   if (!y_finite) {
      return y_negative ? -pi<Float> / 2 : pi<Float> / 2;
   }
   if (!x_finite) {
      if (x_negative) {
         return y_negative ? -pi<Float> : pi<Float>;
      }
      return y_negative ? -Float(0) : Float(0);
   }

   Float const angle = emulated_atan(y / x);
   if (!x_negative) {
      return angle;
   }
   return y_negative ? angle - pi<Float> : angle + pi<Float>;
}

template <is_raw_floating_point Float>
[[nodiscard]]
constexpr auto
emulated_asin(Float argument) -> Float {
   if (is_nan(argument)) {
      return argument;
   }
   if (argument < -1 || argument > 1) {
      return limits<Float>::quiet_NaN();
   }
   if (argument == 1) {
      return pi<Float> / 2;
   }
   if (argument == -1) {
      return -pi<Float> / 2;
   }
   return emulated_atan2(
      argument, static_cast<Float>(emulated_sqrt(1. - argument * argument))
   );
}

template <is_raw_floating_point Float>
[[nodiscard]]
constexpr auto
emulated_acos(Float argument) -> Float {
   if (is_nan(argument)) {
      return argument;
   }
   if (argument < -1 || argument > 1) {
      return limits<Float>::quiet_NaN();
   }
   if (argument == 1) {
      return 0;
   }
   if (argument == -1) {
      return pi<Float>;
   }
   return emulated_atan2(
      static_cast<Float>(emulated_sqrt(1. - argument * argument)), argument
   );
}

template <is_raw_floating_point Float>
[[nodiscard]]
constexpr auto
emulated_sinh(Float argument) -> Float {
   if (is_nan(argument) || !is_finite(argument) || argument == 0) {
      return argument;
   }
   bool const negative = argument < 0;
   Float const magnitude = negative ? -argument : argument;
   if (magnitude < 0.5) {
      Float const squared = magnitude * magnitude;
      Float term = magnitude;
      Float result = magnitude;
      constexpr idx term_count = is_same<Float, float> ? 6u : 10u;
      for (idx index = 1u; index < term_count; ++index) {
         Float const even = Float(make_raw_arithmetic(index * 2u));
         term *= squared / (even * (even + 1));
         result += term;
      }
      return negative ? -result : result;
   }
   Float const exponential = emulated_exp(magnitude);
   Float const result = (exponential - 1 / exponential) / 2;
   return negative ? -result : result;
}

template <is_raw_floating_point Float>
[[nodiscard]]
constexpr auto
emulated_cosh(Float argument) -> Float {
   if (is_nan(argument)) {
      return argument;
   }
   Float const magnitude = argument < 0 ? -argument : argument;
   if (magnitude < 0.5) {
      Float const squared = magnitude * magnitude;
      Float term = 1;
      Float result = 1;
      constexpr idx term_count = is_same<Float, float> ? 6u : 10u;
      for (idx index = 1u; index < term_count; ++index) {
         Float const odd = Float(make_raw_arithmetic(index * 2u - 1u));
         term *= squared / (odd * (odd + 1));
         result += term;
      }
      return result;
   }
   Float const exponential = emulated_exp(magnitude);
   return (exponential + 1 / exponential) / 2;
}

template <is_raw_floating_point Float>
[[nodiscard]]
constexpr auto
emulated_tanh(Float argument) -> Float {
   if (is_nan(argument) || argument == 0) {
      return argument;
   }
   bool const negative = argument < 0;
   Float const magnitude = negative ? -argument : argument;
   if (!is_finite(magnitude)) {
      return negative ? -1 : 1;
   }
   if (magnitude < 0.5) {
      Float const result = emulated_sinh(magnitude) / emulated_cosh(magnitude);
      return negative ? -result : result;
   }
   Float const exponential = emulated_exp(magnitude * 2);
   if (!is_finite(exponential)) {
      return negative ? -1 : 1;
   }
   Float const result = (exponential - 1) / (exponential + 1);
   return negative ? -result : result;
}

}  // namespace cat::detail

namespace cat {

template <is_floating_point Float>
[[nodiscard]]
constexpr auto
sin(Float argument) -> Float {
   if consteval {
      return Float(detail::emulated_sin(make_raw_arithmetic(argument)));
   }
   return Float(__builtin_elementwise_sin(make_raw_arithmetic(argument)));
}

template <is_floating_point Float>
[[nodiscard]]
constexpr auto
cos(Float argument) -> Float {
   if consteval {
      return Float(detail::emulated_cos(make_raw_arithmetic(argument)));
   }
   return Float(__builtin_elementwise_cos(make_raw_arithmetic(argument)));
}

template <is_floating_point Float>
[[nodiscard]]
constexpr auto
tan(Float argument) -> Float {
   if consteval {
      return Float(detail::emulated_tan(make_raw_arithmetic(argument)));
   }
   return Float(__builtin_elementwise_tan(make_raw_arithmetic(argument)));
}

template <is_floating_point Float>
[[nodiscard]]
constexpr auto
asin(Float argument) -> Float {
   if consteval {
      return Float(detail::emulated_asin(make_raw_arithmetic(argument)));
   }
   return Float(__builtin_elementwise_asin(make_raw_arithmetic(argument)));
}

template <is_floating_point Float>
[[nodiscard]]
constexpr auto
acos(Float argument) -> Float {
   if consteval {
      return Float(detail::emulated_acos(make_raw_arithmetic(argument)));
   }
   return Float(__builtin_elementwise_acos(make_raw_arithmetic(argument)));
}

template <is_floating_point Float>
[[nodiscard]]
constexpr auto
atan(Float argument) -> Float {
   if consteval {
      return Float(detail::emulated_atan(make_raw_arithmetic(argument)));
   }
   return Float(__builtin_elementwise_atan(make_raw_arithmetic(argument)));
}

template <is_floating_point Float>
[[nodiscard]]
constexpr auto
atan2(Float y, Float x) -> Float {
   if consteval {
      return Float(
         detail::emulated_atan2(make_raw_arithmetic(y), make_raw_arithmetic(x))
      );
   }
   return Float(__builtin_elementwise_atan2(
      make_raw_arithmetic(y), make_raw_arithmetic(x)
   ));
}

template <is_floating_point Float>
[[nodiscard]]
constexpr auto
sinh(Float argument) -> Float {
   if consteval {
      return Float(detail::emulated_sinh(make_raw_arithmetic(argument)));
   }
   return Float(__builtin_elementwise_sinh(make_raw_arithmetic(argument)));
}

template <is_floating_point Float>
[[nodiscard]]
constexpr auto
cosh(Float argument) -> Float {
   if consteval {
      return Float(detail::emulated_cosh(make_raw_arithmetic(argument)));
   }
   return Float(__builtin_elementwise_cosh(make_raw_arithmetic(argument)));
}

template <is_floating_point Float>
[[nodiscard]]
constexpr auto
tanh(Float argument) -> Float {
   if consteval {
      return Float(detail::emulated_tanh(make_raw_arithmetic(argument)));
   }
   return Float(__builtin_elementwise_tanh(make_raw_arithmetic(argument)));
}

}  // namespace cat
