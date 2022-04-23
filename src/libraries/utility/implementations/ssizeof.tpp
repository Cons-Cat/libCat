// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <utility>

constexpr auto meta::ssizeof(auto const& anything) -> ssize {
    return static_cast<signed int long>(sizeof(anything));
}

template <typename T>
constexpr auto meta::ssizeof() -> ssize {
    return static_cast<signed int long>(sizeof(T));
}
