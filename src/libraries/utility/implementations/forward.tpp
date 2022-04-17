// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <utility>

template <typename T>
constexpr auto std::forward(meta::RemoveReference<T>& input) -> T&& {
    return static_cast<T&&>(input);
}

template <typename T>
constexpr auto std::forward(meta::RemoveReference<T>&& input)
    -> T&& requires(!meta::is_lvalue_reference<T>) {
    return static_cast<T&&>(input);
}
