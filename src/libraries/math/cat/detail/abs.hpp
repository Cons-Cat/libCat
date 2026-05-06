// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/math>

namespace cat {

template <is_arithmetic T>
[[nodiscard]]
constexpr auto
abs(T value) -> T {
   if constexpr (is_unsigned_integral<T>) {
      return value;
   } else {
      return T(__builtin_elementwise_abs(make_raw_arithmetic(value)));
   }
}

}  // namespace cat
