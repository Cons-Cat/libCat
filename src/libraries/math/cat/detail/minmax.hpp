// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/math>

namespace cat {

template <is_less_than_comparable T>
[[nodiscard]]
constexpr auto
min(T value) -> T {
   return value;
}

template <is_less_than_comparable T, typename U, typename... Remaining>
   requires(
      is_implicitly_convertible<U, T>
      && (is_implicitly_convertible<Remaining, T> && ...)
   )
[[nodiscard]]
constexpr auto
min(T value_1, U value_2, Remaining... remaining) -> T {
   T const selected = (value_1 < value_2) ? value_1 : T(value_2);
   return min(selected, remaining...);
}

template <is_greater_than_comparable T>
[[nodiscard]]
constexpr auto
max(T value) -> T {
   return value;
}

template <is_greater_than_comparable T, typename U, typename... Remaining>
   requires(
      is_implicitly_convertible<U, T>
      && (is_implicitly_convertible<Remaining, T> && ...)
   )
[[nodiscard]]
constexpr auto
max(T value_1, U value_2, Remaining... remaining) -> T {
   T const selected = (value_1 > value_2) ? value_1 : T(value_2);
   return max(selected, remaining...);
}

}  // namespace cat
