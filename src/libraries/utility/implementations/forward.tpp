// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/utility>

template <typename T>
constexpr auto std::forward(cat::RemoveReference<T>& input) -> T&& {
    return static_cast<T&&>(input);
}

template <typename T>
    requires(!cat::is_lvalue_reference<T>)
constexpr auto std::forward(cat::RemoveReference<T>&& input) -> T&& {
    return static_cast<T&&>(input);
}
