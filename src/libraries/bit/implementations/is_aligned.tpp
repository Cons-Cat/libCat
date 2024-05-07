// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/bit>

// TODO: Guarantee that the inputs have only a single bit set.

// Returns `true` if `p_value` is aligned to the `alignment` boundary.
template <typename U>
[[nodiscard]]
constexpr auto
cat::is_aligned(U* p_value, uword alignment) -> bool {
    return (uintptr<U>{p_value} & (alignment - 1u)) == 0u;
}

// Returns `true` if `p_value` is aligned to the `alignment` boundary.
template <typename U>
[[nodiscard]]
constexpr auto
cat::is_aligned(uintptr<U> p_value, uword alignment) -> bool {
    return (p_value & (alignment - 1u)) == 0u;
}

// Returns `true` if `value` is aligned to the `alignment` boundary.
template <cat::overflow_policies policy>
[[nodiscard]]
constexpr auto
cat::is_aligned(cat::index<policy> value, cat::uword alignment) -> bool {
    return (uword(value) & (alignment - 1u)) == 0u;
}
