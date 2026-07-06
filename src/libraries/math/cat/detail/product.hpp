// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/math>

namespace cat {

template <is_arithmetic T>
[[nodiscard]]
constexpr auto
product(T value) -> T {
   return value;
}

template <is_arithmetic T, is_arithmetic U, is_arithmetic... Remaining>
   requires(
      is_implicitly_convertible<U, T>
      && (is_implicitly_convertible<Remaining, T> && ...)
   )
[[nodiscard]]
constexpr auto
product(T value_1, U value_2, Remaining... remaining) -> T {
   T const selected = value_1 * T(value_2);
   return product(selected, remaining...);
}

}  // namespace cat
