// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/math>

namespace cat {

template <is_integral T, is_integral U, is_integral V>
[[nodiscard]]
constexpr auto
fma(T value, U multiplier, V addend) {
   return value * multiplier + addend;
}

// Wrap an `fma()` intrinsic in `basic_float` precision policies. The precision
// is determined by `value` in a left-assosciative fashion.
template <is_floating_point T, is_arithmetic U, is_arithmetic V>
[[nodiscard]]
constexpr auto
fma(T value, U multiplier, V addend) {
   return basic_float(value).fma(multiplier, addend);
}

}  // namespace cat
