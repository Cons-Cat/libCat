// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/bit>

// Returns `true` if `p_value` is aligned to the `alignment` boundary.
template <typename U>
[[nodiscard]]
constexpr auto cat::is_aligned(U* p_value, uword alignment) -> bool {
    return (uintptr<U>{p_value} & (alignment - 1u)) == 0u;
}

// Returns `true` if `p_value` is aligned to the `alignment` boundary.
template <typename U>
[[nodiscard]]
constexpr auto cat::is_aligned(uintptr<U> p_value, uword alignment) -> bool {
    return (p_value & (alignment - 1u)) == 0u;
}
