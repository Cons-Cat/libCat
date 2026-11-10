// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// Annex G arithmetic shared by Clang's complex runtime entry points
// (`__mulsc3`, `__divsc3`, `__muldc3`, `__divdc3`). Each entry point takes
// the four real components of `(a + bi) * (c + di)` or
// `(a + bi) / (c + di)` and returns a `_Complex` value. When the plain
// formula produces NaN + NaNi, an infinity is recovered when one operand is
// infinite and the other is a nonzero finite number or an infinity, and
// division by zero produces a signed infinity.

#include <cat/arithmetic>
#include <cat/complex>
#include <cat/math>

namespace cat::detail {

// `floor(log2(|value|))` as a floating value, including the Annex G inputs:
// zero is -infinity, infinity is +infinity, and NaN stays NaN.
[[nodiscard]]
constexpr auto
binary_exponent(float value) noexcept -> float {
   unsigned const bits = __builtin_bit_cast(unsigned, value);
   unsigned const magnitude = bits & 0x7fffffffu;

   if (magnitude > 0x7f800000u) {
      return value;
   }

   if (magnitude == 0x7f800000u) {
      return __builtin_huge_valf();
   }

   if (magnitude == 0u) {
      return -__builtin_huge_valf();
   }

   int const stored = static_cast<int>(magnitude >> 23u);

   if (stored == 0) {
      unsigned const significand = magnitude & 0x7fffffu;
      int const highest = 22 - (__builtin_clz(significand) - 9);
      return static_cast<float>(highest - 149);
   }

   return static_cast<float>(stored - 127);
}

[[nodiscard]]
constexpr auto
binary_exponent(double value) noexcept -> double {
   unsigned long long const bits =
      __builtin_bit_cast(unsigned long long, value);
   unsigned long long const magnitude = bits & 0x7fffffff'ffffffffu;

   if (magnitude > 0x7ff00000'00000000u) {
      return value;
   }

   if (magnitude == 0x7ff00000'00000000u) {
      return __builtin_huge_val();
   }

   if (magnitude == 0u) {
      return -__builtin_huge_val();
   }

   int const stored = static_cast<int>(magnitude >> 52u);

   if (stored == 0) {
      unsigned long long const significand = magnitude & 0xfffff'ffffffffu;
      int const highest = 51 - (__builtin_clzll(significand) - 12);
      return static_cast<double>(highest - 1'074);
   }

   return static_cast<double>(stored - 1'023);
}

// Multiply by a power of two without a libm call. Overflow becomes infinity
// and underflow becomes a subnormal or zero. NaN, infinity, and zero pass
// through.
[[nodiscard]]
constexpr auto
scale_binary(float value, int shift) noexcept -> float {
   unsigned const bits = __builtin_bit_cast(unsigned, value);
   unsigned const sign = bits & 0x80000000u;
   unsigned const magnitude = bits & 0x7fffffffu;

   if (magnitude == 0u || magnitude >= 0x7f800000u) {
      return value;
   }

   int exponent = static_cast<int>(magnitude >> 23u);
   unsigned significand = magnitude & 0x7fffffu;

   if (exponent == 0) {
      int const highest = 22 - (__builtin_clz(significand) - 9);
      int const normalize = __builtin_clz(significand) - 8;
      significand = (significand << normalize) & 0x7fffffu;
      exponent = highest - 22;
   }

   exponent += shift;

   if (exponent >= 0xff) {
      return __builtin_bit_cast(float, sign | 0x7f800000u);
   }

   if (exponent <= 0) {
      unsigned const full = significand | 0x800000u;
      int const right = 1 - exponent;

      // The 24-bit `full` is below half the smallest subnormal, which rounds
      // to zero.
      if (right > 24) {
         return __builtin_bit_cast(float, sign);
      }

      unsigned shifted = full >> right;
      unsigned const round = (full >> (right - 1)) & 1u;
      unsigned const sticky = full & ((1u << (right - 1)) - 1u);

      if (round != 0u && (sticky != 0u || (shifted & 1u) != 0u)) {
         ++shifted;
      }

      if (shifted >= 0x800000u) {
         return __builtin_bit_cast(float, sign | 0x800000u);
      }

      return __builtin_bit_cast(float, sign | shifted);
   }
   return __builtin_bit_cast(
      float, sign | (static_cast<unsigned>(exponent) << 23u) | significand
   );
}

[[nodiscard]]
constexpr auto
scale_binary(double value, int shift) noexcept -> double {
   unsigned long long const bits =
      __builtin_bit_cast(unsigned long long, value);
   unsigned long long const sign = bits & 0x80000000'00000000u;
   unsigned long long const magnitude = bits & 0x7fffffff'ffffffffu;

   if (magnitude == 0u || magnitude >= 0x7ff00000'00000000u) {
      return value;
   }

   int exponent = static_cast<int>(magnitude >> 52u);
   unsigned long long significand = magnitude & 0xfffff'ffffffffu;

   if (exponent == 0) {
      int const highest = 51 - (__builtin_clzll(significand) - 12);
      int const normalize = __builtin_clzll(significand) - 11;
      significand = (significand << normalize) & 0xfffff'ffffffffu;
      exponent = highest - 51;
   }

   exponent += shift;

   if (exponent >= 0x7ff) {
      return __builtin_bit_cast(double, sign | 0x7ff00000'00000000u);
   }

   if (exponent <= 0) {
      unsigned long long const full = significand | 0x100000'00000000u;
      int const right = 1 - exponent;

      // The 53-bit `full` is below half the smallest subnormal, which rounds
      // to zero.
      if (right > 53) {
         return __builtin_bit_cast(double, sign);
      }

      unsigned long long shifted = full >> right;
      unsigned long long const round = (full >> (right - 1)) & 1ull;
      unsigned long long const sticky = full & ((1ull << (right - 1)) - 1ull);

      if (round != 0u && (sticky != 0u || (shifted & 1ull) != 0u)) {
         ++shifted;
      }

      if (shifted >= 0x100000'00000000u) {
         return __builtin_bit_cast(double, sign | 0x100000'00000000u);
      }

      return __builtin_bit_cast(double, sign | shifted);
   }
   return __builtin_bit_cast(
      double,
      sign | (static_cast<unsigned long long>(exponent) << 52u) | significand
   );
}

// Sign-copy for a cat float. The builtin is the boundary: the parts stay
// `basic_float` on either side.
template <is_floating_point Number>
[[nodiscard]]
constexpr auto
copy_sign(Number magnitude, Number sign) noexcept -> Number {
   using raw = raw_arithmetic_type<Number>;

   if constexpr (is_same<raw, float>) {
      return Number(__builtin_copysignf(
         make_raw_arithmetic(magnitude), make_raw_arithmetic(sign)
      ));
   } else {
      return Number(__builtin_copysign(
         make_raw_arithmetic(magnitude), make_raw_arithmetic(sign)
      ));
   }
}

template <is_floating_point Number>
   requires(!is_raw_floating_point<Number>)
[[nodiscard]]
constexpr auto
binary_exponent(Number value) noexcept -> Number {
   return Number(binary_exponent(make_raw_arithmetic(value)));
}

template <is_floating_point Number>
   requires(!is_raw_floating_point<Number>)
[[nodiscard]]
constexpr auto
scale_binary(Number value, int shift) noexcept -> Number {
   return Number(scale_binary(make_raw_arithmetic(value), shift));
}

// The exponent helpers above return a floating value. The scaler wants that
// magnitude as an `int` shift.
template <is_floating_point Number>
   requires(!is_raw_floating_point<Number>)
[[nodiscard]]
constexpr auto
exponent_shift(Number magnitude) noexcept -> int {
   return static_cast<int>(make_raw_arithmetic(magnitude));
}

template <is_floating_point Number>
[[nodiscard]]
constexpr auto
is_infinite(Number value) noexcept -> bool {
   return !is_finite(value) && !is_nan(value);
}

template <is_raw_floating_point T>
[[nodiscard]]
constexpr auto
complex_product(T real_left, T imag_left, T real_right, T imag_right) noexcept {
   using number = basic_float<T>;
   using complex_number = basic_complex_float<T>;

   complex_number const left{number(real_left), number(imag_left)};
   complex_number const right{number(real_right), number(imag_right)};

   number left_real = left.real();
   number left_imag = left.imag();
   number right_real = right.real();
   number right_imag = right.imag();

   number real = left_real * right_real - left_imag * right_imag;
   number imag = left_real * right_imag + left_imag * right_real;

   if (is_nan(real) && is_nan(imag)) {
      // Infinities become ±1 and NaNs become a signed zero. Finite values keep
      // their magnitude, so an infinite operand still produces an infinity.
      bool recalc = false;
      number const one = number(T(1));
      number const zero = number(T(0));

      if (is_infinite(left_real) || is_infinite(left_imag)) {
         left_real = copy_sign(is_infinite(left_real) ? one : zero, left_real);
         left_imag = copy_sign(is_infinite(left_imag) ? one : zero, left_imag);

         if (is_nan(right_real)) {
            right_real = copy_sign(zero, right_real);
         }

         if (is_nan(right_imag)) {
            right_imag = copy_sign(zero, right_imag);
         }

         recalc = true;
      }

      if (is_infinite(right_real) || is_infinite(right_imag)) {
         right_real =
            copy_sign(is_infinite(right_real) ? one : zero, right_real);
         right_imag =
            copy_sign(is_infinite(right_imag) ? one : zero, right_imag);

         if (is_nan(left_real)) {
            left_real = copy_sign(zero, left_real);
         }

         if (is_nan(left_imag)) {
            left_imag = copy_sign(zero, left_imag);
         }

         recalc = true;
      }

      if (
         !recalc
         && (is_infinite(left_real * right_real) || is_infinite(left_imag * right_imag) || is_infinite(left_real * right_imag) || is_infinite(left_imag * right_real))
      ) {
         if (is_nan(left_real)) {
            left_real = copy_sign(zero, left_real);
         }

         if (is_nan(left_imag)) {
            left_imag = copy_sign(zero, left_imag);
         }

         if (is_nan(right_real)) {
            right_real = copy_sign(zero, right_real);
         }

         if (is_nan(right_imag)) {
            right_imag = copy_sign(zero, right_imag);
         }

         recalc = true;
      }

      if (recalc) {
         number const infinite = number(infinity);
         real = infinite * (left_real * right_real - left_imag * right_imag);
         imag = infinite * (left_real * right_imag + left_imag * right_real);
      }
   }

   return __builtin_complex(
      make_raw_arithmetic(real), make_raw_arithmetic(imag)
   );
}

template <is_raw_floating_point T>
[[nodiscard]]
constexpr auto
complex_quotient(
   T real_left, T imag_left, T real_right, T imag_right
) noexcept {
   using number = basic_float<T>;
   using complex_number = basic_complex_float<T>;

   complex_number const left{number(real_left), number(imag_left)};
   complex_number const right{number(real_right), number(imag_right)};

   number left_real = left.real();
   number left_imag = left.imag();
   number right_real = right.real();
   number right_imag = right.imag();

   number const absolute_real = abs(right_real);
   number const absolute_imag = abs(right_imag);
   number largest = absolute_imag;

   if (
      !is_nan(absolute_real)
      && (is_nan(absolute_imag) || absolute_real > absolute_imag)
   ) {
      largest = absolute_real;
   }

   number const magnitude = binary_exponent(largest);

   int shift = 0;

   if (is_finite(magnitude)) {
      shift = exponent_shift(magnitude);
      right_real = scale_binary(right_real, -shift);
      right_imag = scale_binary(right_imag, -shift);
   }

   number const denominator = right_real * right_real + right_imag * right_imag;
   number real = scale_binary(
      (left_real * right_real + left_imag * right_imag) / denominator, -shift
   );
   number imag = scale_binary(
      (left_imag * right_real - left_real * right_imag) / denominator, -shift
   );

   if (is_nan(real) && is_nan(imag)) {
      number const zero = number(T(0));
      number const one = number(T(1));

      if (denominator == zero && (!is_nan(left_real) || !is_nan(left_imag))) {
         number const infinite = copy_sign(number(infinity), right_real);
         real = infinite * left_real;
         imag = infinite * left_imag;
      } else if (
         (is_infinite(left_real) || is_infinite(left_imag))
         && is_finite(right_real) && is_finite(right_imag)
      ) {
         left_real = copy_sign(is_infinite(left_real) ? one : zero, left_real);
         left_imag = copy_sign(is_infinite(left_imag) ? one : zero, left_imag);

         number const infinite = number(infinity);
         real = infinite * (left_real * right_real + left_imag * right_imag);
         imag = infinite * (left_imag * right_real - left_real * right_imag);
      } else if (
         is_infinite(magnitude) && magnitude > zero && is_finite(left_real)
         && is_finite(left_imag)
      ) {
         right_real =
            copy_sign(is_infinite(right_real) ? one : zero, right_real);
         right_imag =
            copy_sign(is_infinite(right_imag) ? one : zero, right_imag);

         real = zero * (left_real * right_real + left_imag * right_imag);
         imag = zero * (left_imag * right_real - left_real * right_imag);
      }
   }

   return __builtin_complex(
      make_raw_arithmetic(real), make_raw_arithmetic(imag)
   );
}

}  // namespace cat::detail
