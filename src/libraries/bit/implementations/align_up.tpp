// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/bit>

namespace cat {

template <typename T>
[[nodiscard]]
constexpr T* _Nullable align_up(T* _Nullable, uword alignment)
   __attribute__((enable_if(!has_single_bit(alignment),
                            "`alignment` must be a power of two!"))) = delete;

// Returns a value rounded up from `p_value` to the nearest `alignment`
// boundary.
template <typename T>
[[nodiscard]]
constexpr auto
align_up(T* _Nullable p_value, uword alignment) -> T* _Nullable {
   return __builtin_align_up(p_value, alignment.raw);
}

template <is_arithmetic T>
[[nodiscard]]
constexpr T
align_up(T, uword alignment)
   __attribute__((enable_if(!has_single_bit(alignment),
                            "`alignment` must be a power of two!"))) = delete;

// Returns a value rounded up from `value` to the nearest `alignment`
// boundary.
template <is_arithmetic T>
[[nodiscard]]
constexpr auto
align_up(T value, uword alignment) -> T {
   return __builtin_align_up(make_raw_arithmetic(value), alignment.raw);
}

}  // namespace cat
