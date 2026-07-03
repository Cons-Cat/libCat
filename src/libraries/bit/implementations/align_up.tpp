// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/bit>

namespace cat {

// Returns a value rounded up from `p_value` to the nearest `alignment`
// boundary.
template <typename T>
[[nodiscard]]
constexpr auto
align_up(T* _Nullable p_value, ualign alignment) -> T* _Nullable {
   if (p_value == nullptr) {
      return nullptr;
   }
   return __builtin_align_up(p_value, alignment.raw);
}

// Returns a value rounded up from `value` to the nearest `alignment`
// boundary.
template <is_arithmetic T>
[[nodiscard]]
constexpr auto
align_up(T value, ualign alignment) -> T {
   return __builtin_align_up(make_raw_arithmetic(value), alignment.raw);
}

}  // namespace cat
