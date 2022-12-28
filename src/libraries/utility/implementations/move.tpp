// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/utility>

// This is in the `std::` namespace to enable some GCC optimizations and
// warnings.
template <typename T>
constexpr auto std::move(T&& input) -> cat::remove_reference<T>&& {
    return static_cast<cat::remove_reference<T>&&>(input);
}
