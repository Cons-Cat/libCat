// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/modulo.hpp>

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

}  // namespace cat
