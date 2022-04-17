// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <utility>

// This is in the `std::` namespace to enable some GCC optimizations and
// warnings.
template <typename T>
constexpr auto std::move(T&& input) -> meta::RemoveReference<T>&& {
    return static_cast<meta::RemoveReference<T>&&>(input);
}
