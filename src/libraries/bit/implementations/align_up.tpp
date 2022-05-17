// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <bit>

// Returns a value rounded up from `p_value` to the nearest `alignment`
// boundary.
template <typename U>
constexpr auto cat::align_up(U* const p_value, ssize const alignment) -> U* {
    return (intptr<U>{p_value} + (alignment - 1).c()) & (~(alignment - 1));
}

// Returns a value rounded up from `p_value` to the nearest `alignment`
// boundary.
template <typename U>
constexpr auto cat::align_up(intptr<U> const p_value, ssize const alignment)
    -> intptr<U> {
    return (p_value + (alignment - 1).c()) & (~(alignment - 1));
}
