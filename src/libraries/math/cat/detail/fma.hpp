// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/math>

namespace cat::detail {

[[nodiscard]]
constexpr auto
emulated_fma(float value, float multiplier, float addend) noexcept -> float {
   // This sequence depends on each operation being rounded separately.
#pragma clang fp contract(off)
   constexpr __UINT64_TYPE__ sign_mask = 1ull << 63u;
   constexpr __UINT64_TYPE__ exponent_mask = 0x7ff00000'00000000ull;

   // The product of two `float`s is exact in `double`.
   double const product =
      static_cast<double>(value) * static_cast<double>(multiplier);
   double const wide_addend = addend;
   double const sum = product + wide_addend;
   auto sum_bits = __builtin_bit_cast(__UINT64_TYPE__, sum);

   bool const sum_is_inf_or_nan = (sum_bits & exponent_mask) == exponent_mask;
   bool const sum_is_zero = (sum_bits & ~sign_mask) == 0u;
   if (!sum_is_inf_or_nan && !sum_is_zero) {
      // Rounding `sum` to `double` and then to `float` can round twice. 2Sum
      // finds the error of `sum` exactly, so that the sticky bits of `sum` can
      // be corrected before the final rounding.
      auto const product_exponent =
         __builtin_bit_cast(__UINT64_TYPE__, product) & exponent_mask;
      auto const addend_exponent =
         __builtin_bit_cast(__UINT64_TYPE__, wide_addend) & exponent_mask;
      double const error = product_exponent >= addend_exponent
                              ? (sum - product) - wide_addend
                              : (sum - wide_addend) - product;
      auto const error_bits = __builtin_bit_cast(__UINT64_TYPE__, error);

      // Only adjust when the 28 bits below `float` precision are all zero.
      bool const error_is_zero = (error_bits & ~sign_mask) == 0u;
      if (!error_is_zero && (sum_bits & 0xfffffffull) == 0u) {
         if ((sum_bits & sign_mask) != (error_bits & sign_mask)) {
            ++sum_bits;
         } else if ((sum_bits & ((1ull << 52u) - 1u)) != 0u) {
            --sum_bits;
         }
      }
   }

   return static_cast<float>(__builtin_bit_cast(double, sum_bits));
}

}  // namespace cat::detail

namespace cat {

template <is_integral T, is_integral U, is_integral V>
[[nodiscard]]
constexpr auto
fma(T value, U multiplier, V addend) noexcept {
   return value * multiplier + addend;
}

// Wrap an `fma()` intrinsic in `basic_float` precision policies. The precision
// is determined by `value` in a left-assosciative fashion.
template <is_floating_point T, is_arithmetic U, is_arithmetic V>
   requires requires(T value, U multiplier, V addend) {
               basic_float(value).fma(multiplier, addend);
            }
[[nodiscard]]
constexpr auto
fma(T value, U multiplier, V addend) noexcept {
   return basic_float(value).fma(multiplier, addend);
}

}  // namespace cat
