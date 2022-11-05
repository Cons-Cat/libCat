// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/bit>

// Returns a value rounded down from `p_value` to the nearest `alignment`
// boundary.
template <typename U>
[[nodiscard]] constexpr auto cat::align_down(U* p_value, usize alignment)
    -> U* {
    // TODO: Add unary `-` operator to remove `.raw`.
    return uintptr<U>{p_value} & (-alignment.raw);
}

// Returns a value rounded down from `p_value` to the nearest `alignment`
// boundary. This only works for two's complement arithmetic.
template <typename U>
[[nodiscard]] constexpr auto cat::align_down(intptr<U> p_value, usize alignment)
    -> intptr<U> {
    // TODO: Add unary `-` operator to remove `.raw`.
    return p_value & (-alignment.raw);
}

// Returns a value rounded down from `p_value` to the nearest `alignment`
// boundary. This only works for two's complement arithmetic.
template <typename U>
[[nodiscard]] constexpr auto cat::align_down(uintptr<U> p_value,
                                             usize alignment) -> uintptr<U> {
    // TODO: Add unary `-` operator to remove `.raw`.
    return p_value & (-alignment.raw);
}
