// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/bit>

// Returns `true` if `p_value` is aligned to the `alignment` boundary.

namespace cat {

template <typename T>
[[nodiscard]]
constexpr bool
is_aligned(T* _Nullable, uword alignment)
   __attribute__((enable_if(!has_single_bit(alignment),
                            "`alignment` must be a power of two!"))) = delete;

template <typename T>
[[nodiscard]]
constexpr auto
is_aligned(T* _Nullable p_value, uword alignment) -> bool {
   return __builtin_is_aligned(p_value, alignment.raw);
}

template <is_arithmetic T>
[[nodiscard]]
constexpr bool
is_aligned(T value, uword alignment)
   __attribute__((enable_if(!has_single_bit(alignment),
                            "`alignment` must be a power of two!"))) = delete;

// Returns `true` if `p_value` is aligned to the `alignment` boundary.
template <is_arithmetic T>
[[nodiscard]]
constexpr auto
is_aligned(T value, uword alignment) -> bool {
   return __builtin_is_aligned(make_raw_arithmetic(value), alignment.raw);
}

}  // namespace cat
