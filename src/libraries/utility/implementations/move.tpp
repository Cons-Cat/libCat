// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <utility>

// This is in the `std::` namespace to enable some GCC optimizations and
// warnings.
template <typename T>
constexpr auto std::move(T&& input) -> meta::remove_reference_t<T>&& {
    return static_cast<meta::remove_reference_t<T>&&>(input);
}
