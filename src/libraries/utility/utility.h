#pragma once

#include <type_traits.h>

template <typename T>
constexpr auto move(T&& input) -> std::remove_reference_t<T>&& {
    return static_cast<std::remove_reference_t<T>&&>(input);
}

template <typename T>
constexpr auto forward(std::remove_reference_t<T>& input) -> T&& {
    return static_cast<T&&>(input);
}

template <typename T>
constexpr auto forward(std::remove_reference_t<T>&& input) -> T&& {
    static_assert(!std::is_lvalue_reference_v<T>);
    return static_cast<T&&>(input);
}
