// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/math>

namespace cat {

template <is_arithmetic T>
[[nodiscard]]
constexpr auto
clamp(T value, T minimum, T maximum) noexcept -> T {
   T lower_bound = value < minimum ? minimum : value;
   return lower_bound > maximum ? maximum : lower_bound;
}

}  // namespace cat
