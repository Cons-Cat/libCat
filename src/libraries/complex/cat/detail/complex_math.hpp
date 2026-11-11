// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/complex>
#include <cat/math>

// `<cat/math>` functions of complex numbers. Each accepts a
// `cat::basic_complex_float` or a raw `_Complex`, and returns a libCat
// complex. The infinities, NaNs, and signed zeros of the results follow C's
// Annex G. Every function is a constant expression, because it is built from
// `<cat/math>` functions that are.
//
// The functions keep the quantity rules of their real counterparts:
// `cat::abs()` keeps the parts' quantity, `cat::arg()`, `cat::asin()`,
// `cat::acos()`, and `cat::atan()` produce radians, `cat::sin()`,
// `cat::cos()`, and `cat::tan()` take radians or degrees, roots and
// `cat::pow<numerator, denominator>()` raise the quantity, and the rest take
// dimensionless numbers.
//
// `cat::min()`, `cat::max()`, `cat::clamp()`, `cat::floor()`, `cat::ceil()`,
// and the rounding and divisibility functions have no complex form, because
// complex numbers are not ordered.

namespace cat {
namespace detail {

// The libCat complex type of a raw `_Complex`, or a libCat complex itself.
template <typename T>
struct complex_class_trait {
   using type = T;
};

template <typename T>
   requires(is_raw_complex<T>)
struct complex_class_trait<T> {
   using type = complex<__typeof_unqual(__real declval<T&>())>;
};

// A complex proxy reference is its value with the reference's policy.
template <typename T>
   requires(is_complex_reference<T>)
struct complex_class_trait<T> {
   using type = complex_reference_view<T>;
};

template <typename T>
using complex_class = complex_class_trait<__typeof_unqual(T)>::type;

template <typename T>
concept is_floating_complex =
   is_complex<T> && is_floating_point<typename complex_class<T>::value_type>;

template <typename T>
concept is_dimensionless_floating_complex =
   is_floating_complex<T>
   && is_dimensionless_arithmetic<typename complex_class<T>::value_type>;

template <typename T>
concept is_angle_complex =
   is_floating_complex<T>
   && is_angle_quantity<typename complex_class<T>::value_type>;

// The parts of a libCat complex as a raw `_Complex`.
template <typename Complex>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
complex_load(Complex const& value) noexcept
   -> complex_raw<typename Complex::component_type> {
   return make_complex_raw<typename Complex::component_type>(
      make_raw_arithmetic(value.real()), make_raw_arithmetic(value.imag())
   );
}

template <typename T>
[[nodiscard]]
constexpr auto
complex_copy_sign(T magnitude, T sign) noexcept -> T {
   if constexpr (is_same<T, float>) {
      return __builtin_copysignf(magnitude, sign);
   } else {
      return __builtin_copysign(magnitude, sign);
   }
}

// `|value|`, which is `+0` for `-0`.
template <typename T>
[[nodiscard]]
constexpr auto
complex_absolute(T value) noexcept -> T {
   return complex_copy_sign(value, T(1));
}

template <typename T>
inline constexpr T complex_nan = T(__builtin_nan(""));

template <typename T>
inline constexpr T complex_infinity = T(__builtin_inf());

// `sqrt(x * x + y * y)` without overflow. An infinite part makes it infinite,
// even when the other part is a NaN.
template <typename T>
[[nodiscard]]
constexpr auto
complex_hypot(T x, T y) noexcept -> T {
   if (__builtin_isinf(x) || __builtin_isinf(y)) {
      return complex_infinity<T>;
   }

   if (__builtin_isnan(x) || __builtin_isnan(y)) {
      return x + y;
   }

   T larger = complex_absolute(x);
   T smaller = complex_absolute(y);
   if (larger < smaller) {
      T const swapped = larger;
      larger = smaller;
      smaller = swapped;
   }

   if (larger == T(0)) {
      return T(0);
   }

   T const ratio = smaller / larger;
   return larger * make_raw_arithmetic(cat::sqrt(T(1) + (ratio * ratio)));
}

template <typename T>
[[nodiscard]]
constexpr auto
complex_arg(complex_raw<T> value) noexcept -> T {
   return make_raw_arithmetic(cat::atan2(__imag value, __real value));
}

template <typename T>
[[nodiscard]]
constexpr auto
complex_log(complex_raw<T> value) noexcept -> complex_raw<T> {
   return make_complex_raw<T>(
      cat::log(complex_hypot(__real value, __imag value)), complex_arg<T>(value)
   );
}

template <typename T>
[[nodiscard]]
constexpr auto
complex_exp(complex_raw<T> value) noexcept -> complex_raw<T> {
   T const real = __real value;
   T imag = __imag value;

   if (imag == T(0)) {
      return make_complex_raw<T>(cat::exp(real), imag);
   }

   if (__builtin_isinf(real)) {
      if (real < T(0)) {
         if (!__builtin_isfinite(imag)) {
            imag = T(1);
         }
      } else if (!__builtin_isfinite(imag)) {
         if (__builtin_isinf(imag)) {
            imag = complex_nan<T>;
         }

         return make_complex_raw<T>(real, imag);
      }
   }

   T const magnitude = cat::exp(real);
   return make_complex_raw<T>(
      magnitude * raw_cos(imag), magnitude * raw_sin(imag)
   );
}

// `value * value`, with one rounding in the real part.
template <typename T>
[[nodiscard]]
constexpr auto
complex_square(complex_raw<T> value) noexcept -> complex_raw<T> {
   T const real = __real value;
   T const imag = __imag value;
   return make_complex_raw<T>(
      (real - imag) * (real + imag), T(2) * real * imag
   );
}

// The principal square root. The real part is never negative, and the
// imaginary part has the sign of `value`'s imaginary part.
template <typename T>
[[nodiscard]]
constexpr auto
complex_sqrt(complex_raw<T> value) noexcept -> complex_raw<T> {
   T real = __real value;
   T imag = __imag value;

   if (__builtin_isinf(imag)) {
      return make_complex_raw<T>(complex_infinity<T>, imag);
   }

   if (__builtin_isinf(real)) {
      if (real > T(0)) {
         return make_complex_raw<T>(
            real, __builtin_isnan(imag) ? imag : complex_copy_sign(T(0), imag)
         );
      }

      return make_complex_raw<T>(
         __builtin_isnan(imag) ? imag : T(0), complex_copy_sign(real, imag)
      );
   }

   if (__builtin_isnan(real) || __builtin_isnan(imag)) {
      return make_complex_raw<T>(complex_nan<T>, complex_nan<T>);
   }

   if (real == T(0) && imag == T(0)) {
      return make_complex_raw<T>(T(0), imag);
   }

   // `|real| + |value|` overflows near the largest finite value. A quarter of
   // `value` has half the root.
   T scale = T(1);
   constexpr T large = limits<T>::max() / T(4);
   if (complex_absolute(real) > large || complex_absolute(imag) > large) {
      real *= T(0.25);
      imag *= T(0.25);
      scale = T(2);
   }

   T const root = make_raw_arithmetic(
      cat::sqrt((complex_absolute(real) + complex_hypot(real, imag)) * T(0.5))
   );

   if (real >= T(0)) {
      return make_complex_raw<T>(scale * root, scale * (imag / (T(2) * root)));
   }

   return make_complex_raw<T>(
      scale * (complex_absolute(imag) / (T(2) * root)),
      scale * complex_copy_sign(root, imag)
   );
}

template <typename T>
[[nodiscard]]
constexpr auto
complex_sinh(complex_raw<T> value) noexcept -> complex_raw<T> {
   T const real = __real value;
   T const imag = __imag value;

   if (__builtin_isinf(real) && !__builtin_isfinite(imag)) {
      return make_complex_raw<T>(real, complex_nan<T>);
   }

   if (real == T(0) && !__builtin_isfinite(imag)) {
      return make_complex_raw<T>(real, complex_nan<T>);
   }

   if (imag == T(0) && !__builtin_isfinite(real)) {
      return value;
   }

   return make_complex_raw<T>(
      cat::sinh(real) * raw_cos(imag), cat::cosh(real) * raw_sin(imag)
   );
}

template <typename T>
[[nodiscard]]
constexpr auto
complex_cosh(complex_raw<T> value) noexcept -> complex_raw<T> {
   T const real = __real value;
   T const imag = __imag value;

   if (__builtin_isinf(real) && !__builtin_isfinite(imag)) {
      return make_complex_raw<T>(complex_absolute(real), complex_nan<T>);
   }

   if (real == T(0) && !__builtin_isfinite(imag)) {
      return make_complex_raw<T>(complex_nan<T>, real);
   }

   if (real == T(0) && imag == T(0)) {
      return make_complex_raw<T>(T(1), imag);
   }

   if (imag == T(0) && !__builtin_isfinite(real)) {
      return make_complex_raw<T>(complex_absolute(real), imag);
   }

   return make_complex_raw<T>(
      cat::cosh(real) * raw_cos(imag), cat::sinh(real) * raw_sin(imag)
   );
}

template <typename T>
[[nodiscard]]
constexpr auto
complex_tanh(complex_raw<T> value) noexcept -> complex_raw<T> {
   T const real = __real value;
   T const imag = __imag value;

   if (__builtin_isinf(real)) {
      T const one = complex_copy_sign(T(1), real);
      if (!__builtin_isfinite(imag)) {
         return make_complex_raw<T>(one, T(0));
      }

      return make_complex_raw<T>(
         one, complex_copy_sign(T(0), raw_sin(T(2) * imag))
      );
   }

   if (__builtin_isnan(real) && imag == T(0)) {
      return value;
   }

   T const twice_real = T(2) * real;
   T const twice_imag = T(2) * imag;
   T const denominator = cat::cosh(twice_real) + raw_cos(twice_imag);
   T const numerator = cat::sinh(twice_real);

   if (__builtin_isinf(numerator) && __builtin_isinf(denominator)) {
      return make_complex_raw<T>(
         numerator > T(0) ? T(1) : T(-1), twice_imag > T(0) ? T(0) : T(-0.)
      );
   }

   return make_complex_raw<T>(
      numerator / denominator, raw_sin(twice_imag) / denominator
   );
}

template <typename T>
[[nodiscard]]
constexpr auto
complex_asinh(complex_raw<T> value) noexcept -> complex_raw<T> {
   T const real = __real value;
   T const imag = __imag value;

   if (__builtin_isinf(real)) {
      if (__builtin_isnan(imag)) {
         return value;
      }

      if (__builtin_isinf(imag)) {
         return make_complex_raw<T>(
            real, complex_copy_sign(pi<T> * T(0.25), imag)
         );
      }

      return make_complex_raw<T>(real, complex_copy_sign(T(0), imag));
   }

   if (__builtin_isnan(real)) {
      if (__builtin_isinf(imag)) {
         return make_complex_raw<T>(imag, real);
      }

      if (imag == T(0)) {
         return value;
      }

      return make_complex_raw<T>(real, real);
   }

   if (__builtin_isinf(imag)) {
      return make_complex_raw<T>(
         complex_copy_sign(imag, real), complex_copy_sign(pi<T> / T(2), imag)
      );
   }

   complex_raw<T> const result =
      complex_log<T>(value + complex_sqrt<T>(complex_square<T>(value) + T(1)));
   return make_complex_raw<T>(
      complex_copy_sign(__real result, real),
      complex_copy_sign(__imag result, imag)
   );
}

template <typename T>
[[nodiscard]]
constexpr auto
complex_atanh(complex_raw<T> value) noexcept -> complex_raw<T> {
   T const real = __real value;
   T const imag = __imag value;

   if (__builtin_isinf(imag)) {
      return make_complex_raw<T>(
         complex_copy_sign(T(0), real), complex_copy_sign(pi<T> / T(2), imag)
      );
   }

   if (__builtin_isnan(imag)) {
      if (__builtin_isinf(real) || real == T(0)) {
         return make_complex_raw<T>(complex_copy_sign(T(0), real), imag);
      }

      return make_complex_raw<T>(imag, imag);
   }

   if (__builtin_isnan(real)) {
      return make_complex_raw<T>(real, real);
   }

   if (__builtin_isinf(real)) {
      return make_complex_raw<T>(
         complex_copy_sign(T(0), real), complex_copy_sign(pi<T> / T(2), imag)
      );
   }

   if (complex_absolute(real) == T(1) && imag == T(0)) {
      return make_complex_raw<T>(
         complex_copy_sign(complex_infinity<T>, real),
         complex_copy_sign(T(0), imag)
      );
   }

   complex_raw<T> const result =
      complex_log<T>((T(1) + value) / (T(1) - value)) / T(2);
   return make_complex_raw<T>(
      complex_copy_sign(__real result, real),
      complex_copy_sign(__imag result, imag)
   );
}

// `asin(z)` is `-i asinh(iz)`.
template <typename T>
[[nodiscard]]
constexpr auto
complex_asin(complex_raw<T> value) noexcept -> complex_raw<T> {
   complex_raw<T> const result =
      complex_asinh<T>(make_complex_raw<T>(-__imag value, __real value));
   return make_complex_raw<T>(__imag result, -__real result);
}

template <typename T>
[[nodiscard]]
constexpr auto
complex_acos(complex_raw<T> value) noexcept -> complex_raw<T> {
   T const real = __real value;
   T const imag = __imag value;

   if (__builtin_isinf(real)) {
      if (__builtin_isnan(imag)) {
         return make_complex_raw<T>(imag, real);
      }

      if (__builtin_isinf(imag)) {
         return make_complex_raw<T>(
            (real < T(0) ? T(0.75) : T(0.25)) * pi<T>, -imag
         );
      }

      if (real < T(0)) {
         return make_complex_raw<T>(
            pi<T>, __builtin_signbit(imag) ? -real : real
         );
      }

      return make_complex_raw<T>(T(0), __builtin_signbit(imag) ? real : -real);
   }

   if (__builtin_isnan(real)) {
      if (__builtin_isinf(imag)) {
         return make_complex_raw<T>(real, -imag);
      }

      return make_complex_raw<T>(real, real);
   }

   if (__builtin_isinf(imag)) {
      return make_complex_raw<T>(pi<T> / T(2), -imag);
   }

   if (real == T(0) && (imag == T(0) || __builtin_isnan(imag))) {
      return make_complex_raw<T>(pi<T> / T(2), -imag);
   }

   complex_raw<T> const result =
      complex_log<T>(value + complex_sqrt<T>(complex_square<T>(value) - T(1)));
   if (__builtin_signbit(imag)) {
      return make_complex_raw<T>(
         complex_absolute(__imag result), complex_absolute(__real result)
      );
   }

   return make_complex_raw<T>(
      complex_absolute(__imag result), -complex_absolute(__real result)
   );
}

// `atan(z)` is `-i atanh(iz)`.
template <typename T>
[[nodiscard]]
constexpr auto
complex_atan(complex_raw<T> value) noexcept -> complex_raw<T> {
   complex_raw<T> const result =
      complex_atanh<T>(make_complex_raw<T>(-__imag value, __real value));
   return make_complex_raw<T>(__imag result, -__real result);
}

// `sin(z)` is `-i sinh(iz)`.
template <typename T>
[[nodiscard]]
constexpr auto
complex_sin(complex_raw<T> value) noexcept -> complex_raw<T> {
   complex_raw<T> const result =
      complex_sinh<T>(make_complex_raw<T>(-__imag value, __real value));
   return make_complex_raw<T>(__imag result, -__real result);
}

// `cos(z)` is `cosh(iz)`.
template <typename T>
[[nodiscard]]
constexpr auto
complex_cos(complex_raw<T> value) noexcept -> complex_raw<T> {
   return complex_cosh<T>(make_complex_raw<T>(-__imag value, __real value));
}

// `tan(z)` is `-i tanh(iz)`.
template <typename T>
[[nodiscard]]
constexpr auto
complex_tan(complex_raw<T> value) noexcept -> complex_raw<T> {
   complex_raw<T> const result =
      complex_tanh<T>(make_complex_raw<T>(-__imag value, __real value));
   return make_complex_raw<T>(__imag result, -__real result);
}

// `exp(exponent * log(base))`.
template <typename T>
[[nodiscard]]
constexpr auto
complex_pow(complex_raw<T> base, complex_raw<T> exponent) noexcept
   -> complex_raw<T> {
   return complex_exp<T>(exponent * complex_log<T>(base));
}

// The principal `root`th root.
template <typename T>
[[nodiscard]]
constexpr auto
complex_nroot(complex_raw<T> value, iword root) noexcept -> complex_raw<T> {
   if (root <= 1) {
      return value;
   }

   return complex_exp<T>(
      complex_log<T>(value) / static_cast<T>(make_raw_arithmetic(root))
   );
}

template <typename T>
[[nodiscard]]
constexpr auto
complex_rnroot(complex_raw<T> value, iword root) noexcept -> complex_raw<T> {
   if (root <= 0) {
      return value;
   }

   return T(1) / complex_nroot<T>(value, root);
}

// `sin(pi * value)` and `cos(pi * value)` as the real and imaginary parts.
// `value` is reduced by halves exactly, so a large `value` stays accurate.
template <typename T>
[[nodiscard]]
constexpr auto
sine_cosine_pi(T value) noexcept -> complex_raw<T> {
   T const halves = cat::floor((T(2) * value) + T(0.5));
   T const reduced = value - (halves * T(0.5));
   int const quadrant =
      static_cast<int>(halves - (T(4) * cat::floor(halves * T(0.25))));
   T const angle = pi<T> * reduced;
   T const sine = raw_sin(angle);
   T const cosine = raw_cos(angle);
   switch (quadrant) {
      case 1:
         return make_complex_raw<T>(cosine, -sine);
      case 2:
         return make_complex_raw<T>(-sine, -cosine);
      case 3:
         return make_complex_raw<T>(-cosine, sine);
      default:
         return make_complex_raw<T>(sine, cosine);
   }
}

// Stirling's series, which is accurate once `value` is at least 7 from the
// negative real axis.
template <typename T>
[[nodiscard]]
constexpr auto
complex_stirling_lgamma(complex_raw<T> value) noexcept -> complex_raw<T> {
   constexpr T half_log_two_pi = T(0x1.d67f1c864beb5p-1);
   complex_raw<T> const reciprocal = T(1) / value;
   complex_raw<T> const squared = reciprocal * reciprocal;

   // `B(2k) / (2k (2k - 1))`, from the last term to the first.
   complex_raw<T> series = make_complex_raw<T>(T(-3617. / 122400.), T(0));
   series = (series * squared) + T(1. / 156.);
   series = (series * squared) - T(691. / 360360.);
   series = (series * squared) + T(1. / 1188.);
   series = (series * squared) - T(1. / 1680.);
   series = (series * squared) + T(1. / 1260.);
   series = (series * squared) - T(1. / 360.);
   series = (series * squared) + T(1. / 12.);

   return ((value - T(0.5)) * complex_log<T>(value)) - value + half_log_two_pi
          + (series * reciprocal);
}

// The principal branch of `log(gamma(value))`, which is continuous everywhere
// except across the negative real axis. Its real part is `cat::lgamma()` of
// the magnitude, and `lgamma(value + 1)` is `lgamma(value) + log(value)`.
template <typename T>
[[nodiscard]]
constexpr auto
complex_lgamma(complex_raw<T> value) noexcept -> complex_raw<T> {
   T const real = __real value;
   T const imag = __imag value;

   if (__builtin_isnan(real) || __builtin_isnan(imag)) {
      return make_complex_raw<T>(complex_nan<T>, complex_nan<T>);
   }

   // Toward an imaginary infinity, `gamma` vanishes and its phase grows.
   if (__builtin_isinf(imag)) {
      return make_complex_raw<T>(
         real == complex_infinity<T> ? real : -complex_infinity<T>, imag
      );
   }

   if (__builtin_isinf(real)) {
      // Along the negative real axis, `gamma` has no limit. Its real part
      // follows `cat::lgamma(-infinity)`.
      if (real < T(0)) {
         return make_complex_raw<T>(complex_infinity<T>, complex_nan<T>);
      }

      return make_complex_raw<T>(
         real,
         imag == T(0) ? imag : complex_copy_sign(complex_infinity<T>, imag)
      );
   }

   if (imag == T(0)) {
      if (real > T(0)) {
         return make_complex_raw<T>(cat::lgamma(real), imag);
      }

      // A pole has no limiting phase.
      if (real == cat::floor(real)) {
         return make_complex_raw<T>(complex_infinity<T>, complex_nan<T>);
      }

      // The negative real axis is the branch cut. Its phase is `-pi` times the
      // number of poles between `real` and zero, approached from `+0i`.
      T const poles = cat::floor(-real) + T(1);
      return make_complex_raw<T>(
         cat::lgamma(real), complex_copy_sign(pi<T> * poles, -imag)
      );
   }

   if (complex_absolute(imag) >= T(14) || real >= T(7)) {
      return complex_stirling_lgamma<T>(value);
   }

   if (real < T(0.5)) {
      // Reflection: `gamma(z) gamma(1 - z) = pi / sin(pi z)`. The multiple of
      // `2 pi i` keeps the result on the principal branch.
      complex_raw<T> const sine_cosine = sine_cosine_pi(real);
      T const scaled_imag = pi<T> * imag;
      complex_raw<T> const sine = make_complex_raw<T>(
         __real sine_cosine * cat::cosh(scaled_imag),
         __imag sine_cosine * cat::sinh(scaled_imag)
      );
      T const branch = complex_copy_sign(T(2) * pi<T>, imag)
                       * cat::floor((real * T(0.5)) + T(0.25));
      return make_complex_raw<T>(cat::log(pi<T>), branch) - complex_log<T>(sine)
             - complex_lgamma<T>(make_complex_raw<T>(T(1) - real, -imag));
   }

   // Shift into Stirling's range with `lgamma(z) = lgamma(z + 1) - log(z)`.
   complex_raw<T> shifted = value;
   complex_raw<T> logs = make_complex_raw<T>(T(0), T(0));
   while (__real shifted < T(7)) {
      // Clang cannot constant-evaluate compound assignment to a `_Complex`.
      logs = logs + complex_log<T>(shifted);
      shifted = shifted + T(1);
   }

   return complex_stirling_lgamma<T>(shifted) - logs;
}

}  // namespace detail

// The magnitude, in the quantity of the parts.
template <detail::is_floating_complex T>
[[nodiscard]]
constexpr auto
abs(T value) noexcept -> detail::complex_class<T>::value_type {
   using complex_type = detail::complex_class<T>;
   using raw = complex_type::component_type;
   auto const parts = detail::complex_load(complex_type(value));
   return typename complex_type::value_type(
      detail::complex_hypot<raw>(__real parts, __imag parts)
   );
}

// The phase angle, in radians.
template <detail::is_floating_complex T>
[[nodiscard]]
constexpr auto
arg(T value) noexcept {
   using complex_type = detail::complex_class<T>;
   using raw = complex_type::component_type;
   return detail::attach_quantity<units::radian>(
      basic_float<raw, complex_type::precision_policy>(
         detail::complex_arg<raw>(detail::complex_load(complex_type(value)))
      )
   );
}

template <detail::is_dimensionless_floating_complex T>
[[nodiscard]]
constexpr auto
exp(T value) noexcept -> detail::complex_class<T> {
   using complex_type = detail::complex_class<T>;
   using raw = complex_type::component_type;
   return complex_type(
      detail::complex_exp<raw>(detail::complex_load(complex_type(value)))
   );
}

// The principal logarithm. Its imaginary part is in `[-pi, pi]`.
template <detail::is_dimensionless_floating_complex T>
[[nodiscard]]
constexpr auto
log(T value) noexcept -> detail::complex_class<T> {
   using complex_type = detail::complex_class<T>;
   using raw = complex_type::component_type;
   return complex_type(
      detail::complex_log<raw>(detail::complex_load(complex_type(value)))
   );
}

// The principal square root, in the square root of the parts' quantity.
template <detail::is_floating_complex T>
   requires(
      arithmetic_quantity<typename detail::complex_class<T>::value_type>::scale
         % 2
      == 0
   )
[[nodiscard]]
constexpr auto
sqrt(T value) noexcept {
   using complex_type = detail::complex_class<T>;
   using raw = complex_type::component_type;
   using part =
      decltype(cat::sqrt(declval<typename complex_type::value_type>()));
   return complex<part>(
      detail::complex_sqrt<raw>(detail::complex_load(complex_type(value)))
   );
}

template <detail::is_floating_complex T>
   requires(
      arithmetic_quantity<typename detail::complex_class<T>::value_type>::scale
         % 2
      == 0
   )
[[nodiscard]]
constexpr auto
rsqrt(T value) noexcept {
   using complex_type = detail::complex_class<T>;
   using raw = complex_type::component_type;
   using part =
      decltype(cat::rsqrt(declval<typename complex_type::value_type>()));
   return complex<part>(
      raw(1)
      / detail::complex_sqrt<raw>(detail::complex_load(complex_type(value)))
   );
}

// The principal `root`th root.
template <detail::is_dimensionless_floating_complex T>
[[nodiscard]]
constexpr auto
nroot(T value, iword root) noexcept -> detail::complex_class<T> {
   using complex_type = detail::complex_class<T>;
   using raw = complex_type::component_type;
   return complex_type(
      detail::complex_nroot<raw>(
         detail::complex_load(complex_type(value)), root
      )
   );
}

template <int root, detail::is_floating_complex T>
   requires(
      root != 0
      && arithmetic_quantity<
               typename detail::complex_class<T>::value_type>::scale
               % root
            == 0
   )
[[nodiscard]]
constexpr auto
nroot(T value) noexcept {
   using complex_type = detail::complex_class<T>;
   using raw = complex_type::component_type;
   using part =
      decltype(cat::nroot<root>(declval<typename complex_type::value_type>()));
   return complex<part>(detail::complex_nroot<raw>(
      detail::complex_load(complex_type(value)), root
   ));
}

template <detail::is_floating_complex T>
   requires(
      arithmetic_quantity<typename detail::complex_class<T>::value_type>::scale
         % 3
      == 0
   )
[[nodiscard]]
constexpr auto
cbrt(T value) noexcept {
   return cat::nroot<3>(value);
}

template <detail::is_dimensionless_floating_complex T>
[[nodiscard]]
constexpr auto
rnroot(T value, iword root) noexcept -> detail::complex_class<T> {
   using complex_type = detail::complex_class<T>;
   using raw = complex_type::component_type;
   return complex_type(
      detail::complex_rnroot<raw>(
         detail::complex_load(complex_type(value)), root
      )
   );
}

template <int root, detail::is_floating_complex T>
   requires(
      root != 0
      && arithmetic_quantity<
               typename detail::complex_class<T>::value_type>::scale
               % root
            == 0
   )
[[nodiscard]]
constexpr auto
rnroot(T value) noexcept {
   using complex_type = detail::complex_class<T>;
   using raw = complex_type::component_type;
   using part =
      decltype(cat::rnroot<root>(declval<typename complex_type::value_type>()));
   return complex<part>(detail::complex_rnroot<raw>(
      detail::complex_load(complex_type(value)), root
   ));
}

template <detail::is_floating_complex T>
   requires(
      arithmetic_quantity<typename detail::complex_class<T>::value_type>::scale
         % 3
      == 0
   )
[[nodiscard]]
constexpr auto
rcbrt(T value) noexcept {
   return cat::rnroot<3>(value);
}

namespace detail {

template <typename T>
concept is_dimensionless_complex =
   is_complex<T>
   && is_dimensionless_arithmetic<typename complex_class<T>::value_type>;

// `base` to an integer power by squaring. A negative `exponent` divides one
// by the power.
template <typename Complex, typename Exponent>
[[nodiscard]]
constexpr auto
complex_integer_power(Complex base, Exponent exponent) noexcept -> Complex {
   using raw_exponent = raw_arithmetic_type<Exponent>;
   using magnitude_type = make_unsigned_type<raw_exponent>;
   using part = Complex::value_type;
   using component = Complex::component_type;
   raw_exponent const stored = make_raw_arithmetic(exponent);
   bool negative = false;
   if constexpr (is_signed<raw_exponent>) {
      negative = stored < 0;
   }

   auto magnitude = static_cast<magnitude_type>(
      negative ? magnitude_type(0) - magnitude_type(stored)
               : magnitude_type(stored)
   );
   Complex result(part(component(1)));
   while (magnitude != 0u) {
      if ((magnitude & 1u) != 0u) {
         result *= base;
      }

      magnitude = static_cast<magnitude_type>(magnitude >> 1u);
      if (magnitude != 0u) {
         base *= base;
      }
   }

   if (negative) {
      return Complex(part(component(1))) / result;
   }

   return result;
}

// The scalar complex type of `T`'s component type and policy.
template <typename T>
using complex_scalar_class = complex<
   raw_arithmetic_type<typename complex_class<T>::value_type>,
   complex_class<T>::policy>;

}  // namespace detail

// `base` to an integer power, by repeated multiplication. This is exact for
// integer complex numbers and small floating ones.
template <detail::is_dimensionless_complex T, is_integral U>
[[nodiscard]]
constexpr auto
pow(T base, U exponent) noexcept -> detail::complex_class<T> {
   using complex_type = detail::complex_class<T>;
   return detail::complex_integer_power(complex_type(base), exponent);
}

// `exp(exponent * log(base))`, the principal power.
template <
   detail::is_dimensionless_floating_complex T,
   detail::is_dimensionless_floating_complex U>
   requires(
      sizeof(typename detail::complex_class<T>::component_type)
      == sizeof(typename detail::complex_class<U>::component_type)
   )
[[nodiscard]]
constexpr auto
pow(T base, U exponent) noexcept -> detail::complex_class<T> {
   using complex_type = detail::complex_class<T>;
   using raw = complex_type::component_type;
   return complex_type(
      detail::complex_pow<raw>(
         detail::complex_load(complex_type(base)),
         detail::complex_load(detail::complex_class<U>(exponent))
      )
   );
}

template <detail::is_dimensionless_floating_complex T, is_floating_point U>
   requires(
      is_dimensionless_arithmetic<U>
      && sizeof(typename detail::complex_class<T>::component_type)
            == sizeof(raw_arithmetic_type<U>)
   )
[[nodiscard]]
constexpr auto
pow(T base, U exponent) noexcept -> detail::complex_class<T> {
   using complex_type = detail::complex_class<T>;
   using raw = complex_type::component_type;
   return complex_type(
      detail::complex_pow<raw>(
         detail::complex_load(complex_type(base)),
         detail::make_complex_raw<raw>(make_raw_arithmetic(exponent), raw(0))
      )
   );
}

template <is_floating_point T, detail::is_dimensionless_floating_complex U>
   requires(
      is_dimensionless_arithmetic<T>
      && sizeof(raw_arithmetic_type<T>)
            == sizeof(typename detail::complex_class<U>::component_type)
   )
[[nodiscard]]
constexpr auto
pow(T base, U exponent) noexcept -> detail::complex_class<U> {
   using complex_type = detail::complex_class<U>;
   using raw = complex_type::component_type;
   return complex_type(
      detail::complex_pow<raw>(
         detail::make_complex_raw<raw>(make_raw_arithmetic(base), raw(0)),
         detail::complex_load(complex_type(exponent))
      )
   );
}

// `base` to `numerator / denominator`, in that power of the parts' quantity.
template <int numerator, int denominator = 1, is_complex T>
   requires(
      denominator != 0 && (detail::is_floating_complex<T> || denominator == 1)
      && (arithmetic_quantity<
             typename detail::complex_class<T>::value_type>::scale
          * numerator)
               % denominator
            == 0
   )
[[nodiscard]]
constexpr auto
pow(T base) noexcept {
   using complex_type = detail::complex_class<T>;
   using raw = complex_type::component_type;
   using part = decltype(cat::pow<numerator, denominator>(
      declval<typename complex_type::value_type>()
   ));
   using scalar_type = detail::complex_scalar_class<T>;
   scalar_type const scalar = detail::complex_load(complex_type(base));
   if constexpr (denominator == 1) {
      return complex<part>(
         detail::complex_load(detail::complex_integer_power(scalar, numerator))
      );
   } else {
      return complex<part>(detail::complex_pow<raw>(
         detail::complex_load(scalar),
         detail::make_complex_raw<raw>(
            static_cast<raw>(numerator) / static_cast<raw>(denominator), raw(0)
         )
      ));
   }
}

// `sin`, `cos`, and `tan` take radians or degrees, like their real
// counterparts.
template <detail::is_angle_complex T>
[[nodiscard]]
constexpr auto
sin(T value) noexcept {
   using complex_type = detail::complex_class<T>;
   using raw = complex_type::component_type;
   complex_type const number = value;
   return basic_complex_float<raw, complex_type::precision_policy>(
      detail::complex_sin<raw>(detail::make_complex_raw<raw>(
         make_raw_arithmetic(detail::make_radian_argument(number.real())),
         make_raw_arithmetic(detail::make_radian_argument(number.imag()))
      ))
   );
}

template <detail::is_floating_complex T>
   requires(!detail::is_angle_complex<T>)
constexpr void sin(T) =
   delete ("`cat::sin` input must be a `radians` or `degrees` quantity!");

template <detail::is_angle_complex T>
[[nodiscard]]
constexpr auto
cos(T value) noexcept {
   using complex_type = detail::complex_class<T>;
   using raw = complex_type::component_type;
   complex_type const number = value;
   return basic_complex_float<raw, complex_type::precision_policy>(
      detail::complex_cos<raw>(detail::make_complex_raw<raw>(
         make_raw_arithmetic(detail::make_radian_argument(number.real())),
         make_raw_arithmetic(detail::make_radian_argument(number.imag()))
      ))
   );
}

template <detail::is_floating_complex T>
   requires(!detail::is_angle_complex<T>)
constexpr void cos(T) =
   delete ("`cat::cos` input must be a `radians` or `degrees` quantity!");

template <detail::is_angle_complex T>
[[nodiscard]]
constexpr auto
tan(T value) noexcept {
   using complex_type = detail::complex_class<T>;
   using raw = complex_type::component_type;
   complex_type const number = value;
   return basic_complex_float<raw, complex_type::precision_policy>(
      detail::complex_tan<raw>(detail::make_complex_raw<raw>(
         make_raw_arithmetic(detail::make_radian_argument(number.real())),
         make_raw_arithmetic(detail::make_radian_argument(number.imag()))
      ))
   );
}

template <detail::is_floating_complex T>
   requires(!detail::is_angle_complex<T>)
constexpr void tan(T) =
   delete ("`cat::tan` input must be a `radians` or `degrees` quantity!");

namespace detail {

// The complex type of radians with `T`'s representation.
template <typename T>
using complex_radian_class = complex<decltype(attach_quantity<units::radian>(
   declval<typename complex_class<T>::value_type>()
))>;

}  // namespace detail

// `asin`, `acos`, and `atan` produce radians, like their real counterparts.
template <detail::is_dimensionless_floating_complex T>
[[nodiscard]]
constexpr auto
asin(T value) noexcept -> detail::complex_radian_class<T> {
   using complex_type = detail::complex_class<T>;
   using raw = complex_type::component_type;
   return detail::complex_radian_class<T>(
      detail::complex_asin<raw>(detail::complex_load(complex_type(value)))
   );
}

template <detail::is_floating_complex T>
   requires(!detail::is_dimensionless_floating_complex<T>)
constexpr void asin(T) = delete ("`cat::asin` input must be dimensionless!");

template <detail::is_dimensionless_floating_complex T>
[[nodiscard]]
constexpr auto
acos(T value) noexcept -> detail::complex_radian_class<T> {
   using complex_type = detail::complex_class<T>;
   using raw = complex_type::component_type;
   return detail::complex_radian_class<T>(
      detail::complex_acos<raw>(detail::complex_load(complex_type(value)))
   );
}

template <detail::is_floating_complex T>
   requires(!detail::is_dimensionless_floating_complex<T>)
constexpr void acos(T) = delete ("`cat::acos` input must be dimensionless!");

template <detail::is_dimensionless_floating_complex T>
[[nodiscard]]
constexpr auto
atan(T value) noexcept -> detail::complex_radian_class<T> {
   using complex_type = detail::complex_class<T>;
   using raw = complex_type::component_type;
   return detail::complex_radian_class<T>(
      detail::complex_atan<raw>(detail::complex_load(complex_type(value)))
   );
}

template <detail::is_floating_complex T>
   requires(!detail::is_dimensionless_floating_complex<T>)
constexpr void atan(T) = delete ("`cat::atan` input must be dimensionless!");

template <detail::is_dimensionless_floating_complex T>
[[nodiscard]]
constexpr auto
sinh(T value) noexcept -> detail::complex_class<T> {
   using complex_type = detail::complex_class<T>;
   using raw = complex_type::component_type;
   return complex_type(
      detail::complex_sinh<raw>(detail::complex_load(complex_type(value)))
   );
}

template <detail::is_dimensionless_floating_complex T>
[[nodiscard]]
constexpr auto
cosh(T value) noexcept -> detail::complex_class<T> {
   using complex_type = detail::complex_class<T>;
   using raw = complex_type::component_type;
   return complex_type(
      detail::complex_cosh<raw>(detail::complex_load(complex_type(value)))
   );
}

template <detail::is_dimensionless_floating_complex T>
[[nodiscard]]
constexpr auto
tanh(T value) noexcept -> detail::complex_class<T> {
   using complex_type = detail::complex_class<T>;
   using raw = complex_type::component_type;
   return complex_type(
      detail::complex_tanh<raw>(detail::complex_load(complex_type(value)))
   );
}

// The principal branch of `log(gamma(value))`.
template <detail::is_dimensionless_floating_complex T>
[[nodiscard]]
constexpr auto
lgamma(T value) noexcept -> detail::complex_class<T> {
   using complex_type = detail::complex_class<T>;
   using raw = complex_type::component_type;
   return complex_type(
      detail::complex_lgamma<raw>(detail::complex_load(complex_type(value)))
   );
}

namespace detail {

template <typename T>
concept is_complex_operand_of_math = is_arithmetic<T> || is_complex<T>;

}  // namespace detail

// `value * multiplier + addend`. A complex product is not fused.
template <
   detail::is_complex_operand_of_math T, detail::is_complex_operand_of_math U,
   detail::is_complex_operand_of_math V>
   requires(
      (detail::is_basic_complex<T> || detail::is_basic_complex<U>
       || detail::is_basic_complex<V>)
      && requires(
         T value, U multiplier, V addend
      ) { value * multiplier + addend; }
   )
[[nodiscard]]
constexpr auto
fma(T value, U multiplier, V addend) noexcept {
   return value * multiplier + addend;
}

template <
   detail::is_complex_operand_of_math T,
   detail::is_complex_operand_of_math... Remaining>
   requires(
      detail::is_basic_complex<T>
      || (detail::is_basic_complex<Remaining> || ...)
   )
[[nodiscard]]
constexpr auto
sum(T value, Remaining... remaining) noexcept {
   return (value + ... + remaining);
}

template <
   detail::is_complex_operand_of_math T,
   detail::is_complex_operand_of_math... Remaining>
   requires(
      detail::is_basic_complex<T>
      || (detail::is_basic_complex<Remaining> || ...)
   )
[[nodiscard]]
constexpr auto
product(T value, Remaining... remaining) noexcept {
   return (value * ... * remaining);
}

}  // namespace cat
