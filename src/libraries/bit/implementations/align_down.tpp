// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <bit>

// Returns a value rounded down from `p_value` to the nearest `alignment`
// boundary.
template <typename U>
constexpr auto cat::align_down(U* const p_value, ssize const alignment) -> U* {
    return intptr<U>{p_value} & (~(alignment - 1));
}

// Returns a value rounded down from `p_value` to the nearest `alignment`
// boundary.
template <typename U>
constexpr auto cat::align_down(intptr<U> const p_value, ssize const alignment)
    -> intptr<U> {
    return p_value & (~(alignment - 1));
}
