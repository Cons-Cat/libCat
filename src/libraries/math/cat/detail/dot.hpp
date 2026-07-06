// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/math>

namespace cat {

template <is_arithmetic T, is_arithmetic U>
[[nodiscard]]
constexpr auto
dot(T value_1, U value_2) {
   return value_1 * value_2;
}

template <
   is_arithmetic T, is_arithmetic U, is_arithmetic V, is_arithmetic W,
   is_arithmetic... Remaining>
[[nodiscard]]
constexpr auto
dot(T value_1, U value_2, V value_3, W value_4, Remaining... remaining) {
   return dot(value_1, value_2) + dot(value_3, value_4, remaining...);
}

}  // namespace cat
