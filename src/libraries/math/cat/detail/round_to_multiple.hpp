// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/math>

namespace cat {

template <is_integral T, is_implicitly_convertible<T> U>
[[nodiscard]]
constexpr auto
round_up_to_multiple_of(T value, U multiple) -> T {
   if (multiple == T(0)) {
      return value;
   }

   T remainder = value % multiple;

   if (remainder == T(0)) {
      return value;
   }

   return value + (multiple - remainder);
}

template <is_integral T, is_implicitly_convertible<T> U>
[[nodiscard]]
constexpr auto
round_down_to_multiple_of(T value, U multiple) -> T {
   if (multiple == T(0)) {
      return value;
   }

   T remainder = value % multiple;

   if (remainder == T(0)) {
      return value;
   }

   return value - remainder;
}

template <is_integral T, is_implicitly_convertible<T> U>
[[nodiscard]]
constexpr auto
round_to_multiple_of(T value, U multiple) -> T {
   if (multiple == T(0)) {
      return value;
   }

   T remainder = value % multiple;

   if (remainder == T(0)) {
      return value;
   }

   // If remainder is more than or equal to half of multiple, round up, else
   // round down.
   if (remainder >= multiple / T(2)) {
      return value + (multiple - remainder);
   }
   return value - remainder;
}

}  // namespace cat
