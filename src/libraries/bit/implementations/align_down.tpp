// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/bit>

namespace cat {

// Returns a value rounded down from `p_value` to the nearest `alignment`
// boundary.
template <typename T>
[[nodiscard]]
constexpr auto
align_down(T* _Nullable p_value, ualign alignment) -> T* _Nullable {
   if (p_value == nullptr) {
      return nullptr;
   }
   return __builtin_align_down(p_value, alignment.raw);
}

// Returns a value rounded down from `value` to the nearest `alignment`
// boundary.
template <is_arithmetic T>
[[nodiscard]]
constexpr auto
align_down(T value, ualign alignment) -> T {
   return __builtin_align_down(make_raw_arithmetic(value), alignment.raw);
}

}  // namespace cat
