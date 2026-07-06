// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/math>

namespace cat {

template <is_integral T, is_integral U>
[[nodiscard]]
constexpr auto
div_ceil(T dividend, U divisor) -> T {
   using raw_type = raw_arithmetic_type<T>;
   raw_type const raw_dividend = make_raw_arithmetic(dividend);
   raw_type const raw_divisor =
      static_cast<raw_type>(make_raw_arithmetic(divisor));
   return T(
      static_cast<raw_type>(
         (raw_dividend + raw_divisor - raw_type(1)) / raw_divisor
      )
   );
}

template <is_integral T, is_integral U>
[[nodiscard]]
constexpr auto
div_floor(T dividend, U divisor) -> T {
   using raw_type = raw_arithmetic_type<T>;
   raw_type const raw_dividend = make_raw_arithmetic(dividend);
   raw_type const raw_divisor =
      static_cast<raw_type>(make_raw_arithmetic(divisor));
   raw_type quotient = raw_dividend / raw_divisor;
   raw_type const remainder = raw_dividend % raw_divisor;

   // NOLINTBEGIN(bugprone-branch-clone)
   if constexpr (is_signed<raw_type>) {
      if (
         remainder != raw_type(0)
         && ((remainder < raw_type(0)) != (raw_divisor < raw_type(0)))
      ) {
         --quotient;
      }
   }

   return T(quotient);
}

// `ceil` and `floor` themselves are exact, but the underlying `a/b` divide
// honours `T`'s precision policy (precise vs fast).
template <is_floating_point T, is_arithmetic U>
[[nodiscard]]
constexpr auto
div_ceil(T dividend, U divisor) -> T {
   using raw_type = raw_arithmetic_type<T>;
   raw_type const raw_dividend = make_raw_arithmetic(dividend);
   raw_type const raw_divisor =
      static_cast<raw_type>(make_raw_arithmetic(divisor));
   if constexpr (make_precision_policy<T> == precision_policies::precise) {
#pragma float_control(precise, on)
      return T(__builtin_elementwise_ceil(raw_dividend / raw_divisor));
   } else {
#pragma float_control(precise, off)
      return T(__builtin_elementwise_ceil(raw_dividend / raw_divisor));
   }
   // NOLINTEND(bugprone-branch-clone)
}

template <is_floating_point T, is_arithmetic U>
[[nodiscard]]
constexpr auto
div_floor(T dividend, U divisor) -> T {
   using raw_type = raw_arithmetic_type<T>;
   raw_type const raw_dividend = make_raw_arithmetic(dividend);
   raw_type const raw_divisor =
      static_cast<raw_type>(make_raw_arithmetic(divisor));
   // NOLINTBEGIN(bugprone-branch-clone)
   if constexpr (make_precision_policy<T> == precision_policies::precise) {
#pragma float_control(precise, on)
      return T(__builtin_elementwise_floor(raw_dividend / raw_divisor));
   } else {
#pragma float_control(precise, off)
      return T(__builtin_elementwise_floor(raw_dividend / raw_divisor));
   }
   // NOLINTEND(bugprone-branch-clone)
}

}  // namespace cat
