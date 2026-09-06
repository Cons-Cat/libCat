// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/math>

namespace cat {

template <is_arithmetic T, is_arithmetic U, is_arithmetic... Remaining>
   requires(sizeof...(Remaining) % 2 == 0)
[[nodiscard]]
constexpr auto
dot(T value_1, U value_2, Remaining... remaining) {
   if constexpr (sizeof...(Remaining) == 0) {
      return value_1 * value_2;
   } else {
      return value_1 * value_2 + dot(remaining...);
   }
}

}  // namespace cat
