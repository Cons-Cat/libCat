// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/exp.hpp>
#include <cat/detail/log.hpp>

namespace cat {
namespace detail {

template <is_integral T, typename U>
constexpr auto
pow_integral(T base, U exponent) -> T {
   if (exponent < 0) {
      return 0;
   }

   T result = 1;
   while (exponent) {
      if (exponent & 1) {
         result *= base;
      }
      exponent >>= 1;
      base *= base;
   }
   return result;
}

template <is_raw_floating_point Float>
[[nodiscard]]
constexpr auto
pow_is_integer(Float value) -> bool {
   constexpr Float exact_integer_limit =
      is_same<Float, float> ? 0x1p24f : 0x1p53;
   if (value >= exact_integer_limit || value <= -exact_integer_limit) {
      return true;
   }
   return value == static_cast<long long>(value);
}

template <is_raw_floating_point Float>
[[nodiscard]]
constexpr auto
pow_is_odd_integer(Float value) -> bool {
   constexpr Float exact_integer_limit =
      is_same<Float, float> ? 0x1p24f : 0x1p53;
   if (value >= exact_integer_limit || value <= -exact_integer_limit) {
      return false;
   }
   return (static_cast<long long>(value) & 1ll) != 0;
}

template <is_raw_floating_point Float>
[[nodiscard]]
constexpr auto
emulated_pow(Float base, Float exponent) -> Float {
   if (exponent == 0 || base == 1) {
      return 1;
   }
   if (is_nan(base) || is_nan(exponent)) {
      return limits<Float>::quiet_NaN();
   }

   Float const magnitude = abs(base);
   if (!is_finite(exponent)) {
      if (magnitude == 1) {
         return 1;
      }
      bool const grows = magnitude > 1;
      return grows == (exponent > 0) ? Float(infinity) : 0;
   }

   bool const odd_exponent = pow_is_odd_integer(exponent);
   if (!is_finite(base)) {
      if (base < 0 && !pow_is_integer(exponent)) {
         return limits<Float>::quiet_NaN();
      }
      Float const result = exponent > 0 ? Float(infinity) : 0;
      return base < 0 && odd_exponent ? -result : result;
   }

   if (base == 0) {
      Float const result = exponent > 0 ? 0 : Float(infinity);
      return __builtin_signbit(base) != 0 && odd_exponent ? -result : result;
   }

   bool const negative = base < 0;
   if (negative && !pow_is_integer(exponent)) {
      return limits<Float>::quiet_NaN();
   }

   Float const result = emulated_exp(exponent * emulated_log(magnitude));
   return negative && odd_exponent ? -result : result;
}

}  // namespace detail

template <is_integral T, is_integral U>
[[nodiscard]]
constexpr auto
pow(T base, U exponent) -> T {
   return detail::pow_integral(base, exponent);
}

template <is_floating_point T, is_arithmetic U>
   requires(
      is_integral<U>
      || (is_floating_point<U> && sizeof(raw_arithmetic_type<U>) == sizeof(raw_arithmetic_type<T>))
   )
[[nodiscard]]
constexpr auto
pow(T base, U exponent) -> T {
   using raw_type = raw_arithmetic_type<T>;
   if consteval {
      return T(
         detail::emulated_pow(
            make_raw_arithmetic(base), static_cast<raw_type>(exponent)
         )
      );
   }
   return T(__builtin_elementwise_pow(
      make_raw_arithmetic(base), static_cast<raw_type>(exponent)
   ));
}

}  // namespace cat
