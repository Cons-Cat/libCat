// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <bit>

// Returns `true` if `p_value` is aligned to the `alignment` boundary.
template <typename U>
constexpr auto cat::is_aligned(U* const p_value, ssize const alignment)
    -> bool {
    return (intptr<U>{p_value} & (alignment - 1)) == 0;
}

// Returns `true` if `p_value` is aligned to the `alignment` boundary.
template <typename U>
constexpr auto cat::is_aligned(intptr<U> const p_value, ssize const alignment)
    -> bool {
    return (p_value & (alignment - 1)) == 0;
}
