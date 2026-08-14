// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/math>

namespace cat::detail {

[[nodiscard]]
constexpr auto
emulated_lgamma(double argument) -> double {
   if (is_nan(argument)) {
      return argument;
   }
   if (!is_finite(argument)) {
      return infinity;
   }
   if (argument == 0. || argument <= -0x1p52) {
      return infinity;
   }
   if (
      argument < 0.
      && argument == static_cast<double>(static_cast<long long>(argument))
   ) {
      return infinity;
   }

   bool const reflected = argument < 0.5;
   double reflection = 0.;
   if (reflected) {
      double const sine = abs(emulated_sin(pi<double> * argument));
      reflection = emulated_log(pi<double> / sine);
      argument = 1. - argument;
   }

   double shifted = argument;
   double correction = 0.;
   while (shifted < 12.) {
      correction -= emulated_log(shifted);
      shifted += 1.;
   }

   double const reciprocal = 1. / shifted;
   double const squared = reciprocal * reciprocal;
   double series = -691. / 360360.;
   series = (series * squared) + (1. / 156.);
   series = (series * squared) - (3617. / 122400.);
   series = (series * squared) + (1. / 1188.);
   series = (series * squared) - (1. / 1680.);
   series = (series * squared) + (1. / 1260.);
   series = (series * squared) - (1. / 360.);
   series = (series * squared) + (1. / 12.);
   series *= reciprocal;

   constexpr double half_log_two_pi = 0x1.d67f1c864beb5p-1;
   // TODO: Replace this Stirling approximation with a native libCat lgamma.
   double const result = correction + ((shifted - 0.5) * emulated_log(shifted))
                         - shifted + half_log_two_pi + series;
   return reflected ? reflection - result : result;
}

}  // namespace cat::detail

namespace cat {

template <is_floating_point Float>
[[nodiscard]]
constexpr auto
lgamma(Float argument) -> Float {
   using raw_type = raw_arithmetic_type<Float>;
   if consteval {
      return Float(detail::emulated_lgamma(static_cast<double>(argument)));
   }
   if constexpr (is_same<raw_type, float>) {
      return Float(__builtin_lgammaf(make_raw_arithmetic(argument)));
   } else {
      return Float(__builtin_lgamma(make_raw_arithmetic(argument)));
   }
}

}  // namespace cat
