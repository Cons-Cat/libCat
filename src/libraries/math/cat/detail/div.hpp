// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/round.hpp>

#include <cat/math>

namespace cat {

template <is_integral T, is_integral U>
[[nodiscard]]
constexpr auto
div_ceil(T dividend, U divisor) {
   using raw_type = raw_arithmetic_type<T>;
   using result_type = decltype(dividend / divisor);
   raw_type const raw_dividend = make_raw_arithmetic(dividend);
   raw_type const raw_divisor = make_raw_arithmetic(divisor);
   return result_type((raw_dividend + raw_divisor - raw_type(1)) / raw_divisor);
}

template <is_integral T, is_integral U>
[[nodiscard]]
constexpr auto
div_floor(T dividend, U divisor) {
   using raw_type = raw_arithmetic_type<T>;
   using result_type = decltype(dividend / divisor);
   raw_type const raw_dividend = make_raw_arithmetic(dividend);
   raw_type const raw_divisor = make_raw_arithmetic(divisor);
   raw_type quotient = raw_dividend / raw_divisor;
   raw_type const remainder = raw_dividend % raw_divisor;

   // NOLINTBEGIN(bugprone-branch-clone)
   if constexpr (is_signed<raw_type>) {
      if (remainder != 0 && ((remainder < 0) != (raw_divisor < 0))) {
         --quotient;
      }
   }

   return result_type(quotient);
}

// `ceil` and `floor` themselves are exact, but the underlying `a/b` divide
// honours `T`'s precision policy (precise vs fast).
template <is_floating_point T, is_arithmetic U>
[[nodiscard]]
constexpr auto
div_ceil(T dividend, U divisor) {
   using raw_type = raw_arithmetic_type<T>;
   using result_type = decltype(dividend / divisor);
   raw_type const raw_dividend = make_raw_arithmetic(dividend);
   raw_type const raw_divisor = make_raw_arithmetic(divisor);
   if constexpr (make_precision_policy<T> == precision_policies::precise) {
#pragma float_control(precise, on)
      return ceil(result_type(raw_dividend / raw_divisor));
   } else {
#pragma float_control(precise, off)
      return ceil(result_type(raw_dividend / raw_divisor));
   }
   // NOLINTEND(bugprone-branch-clone)
}

template <is_floating_point T, is_arithmetic U>
[[nodiscard]]
constexpr auto
div_floor(T dividend, U divisor) {
   using raw_type = raw_arithmetic_type<T>;
   using result_type = decltype(dividend / divisor);
   raw_type const raw_dividend = make_raw_arithmetic(dividend);
   raw_type const raw_divisor = make_raw_arithmetic(divisor);
   // NOLINTBEGIN(bugprone-branch-clone)
   if constexpr (make_precision_policy<T> == precision_policies::precise) {
#pragma float_control(precise, on)
      return floor(result_type(raw_dividend / raw_divisor));
   } else {
#pragma float_control(precise, off)
      return floor(result_type(raw_dividend / raw_divisor));
   }
   // NOLINTEND(bugprone-branch-clone)
}

}  // namespace cat
