// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/math>

namespace cat {
namespace detail {
// TODO: Consider a jump-table implementation.
template <is_integral T, typename U>
constexpr auto
pow_integral(T base, U exponent) -> T {
   if (exponent < 0) {
      // A negative exponent should always make this floor to 0.
      return 0;
   }

   if constexpr (is_signed<T>) {
      base = base < 0 ? -base : base;
   }

   // "Exponentation by squaring" algorithm.
   T result = 1;
   while (exponent) {
      if (exponent & 1) {
         result *= base;
      }
      // Since this is guaranteed positive, it can bitshift.
      exponent >>= 1;
      base *= base;
   }

   return result;
}

// `ln(2)` for emulated `log` and `exp` (range reduction and reconstructing
// `ln`).
inline constexpr float8 ln2 =
   0.69314718055994530941723212145817656807550030136018;

// Emulate floating point math for targets without native instructions. This is
// required for linking Clang math intrinsics.
// TODO: Look into better algorithms. If these are the best, note that.

// ISO C `ldexp` semantics by repeated scaling (no classical power series).
[[nodiscard]]
constexpr auto
emulated_ldexp(double value, int binary_exponent) -> double {
   if (value == 0. || __builtin_isinf(value) || __builtin_isnan(value)) {
      return value;
   }
   if (binary_exponent > 0) {
      for (int step_index = 0; step_index < binary_exponent; ++step_index) {
         value *= 2.;
         if (__builtin_isinf(value)) {
            break;
         }
      }
   } else {
      for (int step_index = 0; step_index < -binary_exponent; ++step_index) {
         value *= 0.5;
         if (value == 0.) {
            break;
         }
      }
   }
   return value;
}

// Maclaurin series for `artanh` (Colin Maclaurin, 1742) with
// `(normalized_mantissa-1)/(normalized_mantissa+1)` after splitting IEEE-754
// exponent and mantissa, then add `unbiased_binary_exponent ln 2`.
[[nodiscard]]
constexpr auto
emulated_log(double argument) -> double {
   if (argument < 0. || __builtin_isnan(argument)) {
      return __builtin_nan("");
   }
   if (argument == 0.) {
      return -__builtin_inf();
   }
   int subnormal_exponent_adjustment = 0;
   while (argument > 0. && argument < 0x1p-1022) {
      argument *= 0x1p52;
      subnormal_exponent_adjustment -= 52;
   }
   unsigned long long const bits_as_uint64 =
      __builtin_bit_cast(unsigned long long, argument);
   int const unbiased_binary_exponent =
      static_cast<int>((bits_as_uint64 >> 52) & 0x7ffu) - 1'023
      + subnormal_exponent_adjustment;
   unsigned long long const mantissa_fraction_bits =
      bits_as_uint64 & ((1ull << 52) - 1);
   unsigned long long const mantissa_with_biased_exponent_bits =
      mantissa_fraction_bits | (0x3ffull << 52);
   double const normalized_mantissa =
      __builtin_bit_cast(double, mantissa_with_biased_exponent_bits);
   double const atanh_kernel_argument =
      (normalized_mantissa - 1.) / (normalized_mantissa + 1.);
   double const atanh_kernel_argument_squared =
      atanh_kernel_argument * atanh_kernel_argument;
   double atanh_series_sum = 0.;
   double odd_power_of_atanh_kernel = atanh_kernel_argument;
   for (int term_index = 0; term_index < 24; ++term_index) {
      atanh_series_sum +=
         odd_power_of_atanh_kernel / static_cast<double>(2 * term_index + 1);
      odd_power_of_atanh_kernel *= atanh_kernel_argument_squared;
   }
   return 2. * atanh_series_sum
          + static_cast<double>(unbiased_binary_exponent) * ln2.raw;
}

// Taylor-Maclaurin series for `exp` (Brook Taylor, 1715, Maclaurin form) on `z`
// minus the nearest multiple of `ln 2`, then scale with `ldexp`.
[[nodiscard]]
constexpr auto
emulated_exp(double argument) -> double {
   if (__builtin_isnan(argument)) {
      return argument;
   }
   // Roughly `ln(DBL_MAX)`. Above this, `exp(argument)` overflows IEEE-754
   // `double`.
   if (argument > 709.78271289338399) {
      return __builtin_inf();
   }
   // Roughly `ln(2^-1074)` (smallest positive subnormal). Below this,
   // `exp(argument)` underflows to zero in `double`.
   if (argument < -745.13321910194122) {
      return 0.;
   }
   double const ln2_multiple_rounded_to_double =
      __builtin_nearbyint(argument / ln2.raw);
   if (ln2_multiple_rounded_to_double > 1024.) {
      return __builtin_inf();
   }
   if (ln2_multiple_rounded_to_double < -1074.) {
      return 0.;
   }
   auto const binary_exponent_for_scale =
      static_cast<int>(ln2_multiple_rounded_to_double);
   double const reduced_argument =
      argument - (ln2_multiple_rounded_to_double * ln2.raw);
   double exponential_series_sum = 1.;
   double maclaurin_term = 1.;
   for (int term_index = 1; term_index <= 24; ++term_index) {
      maclaurin_term *= reduced_argument / static_cast<double>(term_index);
      exponential_series_sum += maclaurin_term;
   }
   return emulated_ldexp(exponential_series_sum, binary_exponent_for_scale);
}

// Euler's formula `x^y = exp(y ln x)` (Leonhard Euler) after IEEE special cases
// and integer-exponent handling when `x` is negative.
[[nodiscard]]
constexpr auto
emulated_pow(double base, double exponent) -> double {
   if (__builtin_isnan(base) || __builtin_isnan(exponent)) {
      return __builtin_nan("");
   }
   if (exponent == 0.) {
      return 1.;
   }
   if (base == 1.) {
      return 1.;
   }
   if (base == -1. && __builtin_isinf(exponent)) {
      return 1.;
   }
   if (__builtin_isinf(base)) {
      if (base < 0.) {
         return __builtin_nan("");
      }
      if (exponent > 0.) {
         return base;
      }
      if (exponent < 0.) {
         return 0.;
      }
      return __builtin_nan("");
   }
   if (base == 0.) {
      if (exponent > 0.) {
         return __builtin_copysign(0., base);
      }
      if (exponent < 0.) {
         return __builtin_copysign(__builtin_inf(), base);
      }
      return 1.;
   }
   if (base < 0.) {
      double const truncated_exponent = __builtin_trunc(exponent);
      if (exponent != truncated_exponent) {
         return __builtin_nan("");
      }
      double const positive_magnitude = -base;
      double const power_of_positive_magnitude =
         emulated_exp(exponent * emulated_log(positive_magnitude));
      long long const truncated_exponent_as_integer =
         static_cast<long long>(truncated_exponent);
      if ((truncated_exponent_as_integer & 1) == 0) {
         return power_of_positive_magnitude;
      }
      return -power_of_positive_magnitude;
   }
   return emulated_exp(exponent * emulated_log(base));
}

// Same as `emulated_pow` (Euler) in `double`, narrowed to `float`.
[[nodiscard]]
constexpr auto
emulated_powf(float base, float exponent) -> float {
   return static_cast<float>(
      emulated_pow(static_cast<double>(base), static_cast<double>(exponent))
   );
}
}  // namespace detail

template <is_integral T, is_integral U>
[[nodiscard]]
constexpr auto
pow(T base, U exponent) -> T {
   return detail::pow_integral(base, make_raw_arithmetic(exponent));
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
   raw_type raw_base = make_raw_arithmetic(base);
   raw_type raw_exponent = static_cast<raw_type>(make_raw_arithmetic(exponent));

   return T(__builtin_elementwise_pow(raw_base, raw_exponent));
}

}  // namespace cat
