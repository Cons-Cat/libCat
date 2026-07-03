// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/bit>

// Returns `true` if `p_value` is aligned to the `alignment` boundary.

namespace cat {

template <typename T>
[[nodiscard]]
constexpr auto
is_aligned(T* _Nullable p_value, ualign alignment) -> bool {
   return __builtin_is_aligned(p_value, alignment.raw);
}

// Returns `true` if `p_value` is aligned to the `alignment` boundary.
template <is_arithmetic T>
[[nodiscard]]
constexpr auto
is_aligned(T value, ualign alignment) -> bool {
   return __builtin_is_aligned(make_raw_arithmetic(value), alignment.raw);
}

}  // namespace cat
