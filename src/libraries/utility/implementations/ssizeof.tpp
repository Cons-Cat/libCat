// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/utility>

constexpr auto cat::ssizeof(auto const& anything) -> ssize {
    return static_cast<signed int long>(sizeof(anything));
}

template <typename T>
constexpr auto cat::ssizeof() -> ssize {
    return static_cast<signed int long>(sizeof(T));
}
