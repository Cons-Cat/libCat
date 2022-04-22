// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// #include <numerals>
// #include <utility>

constexpr auto meta::ssizeof(auto const& anything) -> ssize {
    return static_cast<ssize>(sizeof(anything));
}

template <typename T>
constexpr auto meta::ssizeof() -> ssize {
    return static_cast<ssize>(sizeof(T));
}
