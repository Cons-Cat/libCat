#pragma once

#include <type_traits.h>

namespace meta {

template <typename T>
constexpr auto move(T&& input) -> meta::remove_reference_t<T>&& {
    return static_cast<meta::remove_reference_t<T>&&>(input);
}

template <typename T>
constexpr auto forward(meta::remove_reference_t<T>& input) -> T&& {
    return static_cast<T&&>(input);
}

template <typename T>
constexpr auto forward(meta::remove_reference_t<T>&& input)
    -> T&& requires(!meta::is_lvalue_reference_v<T>) {
    return static_cast<T&&>(input);
}

}  // namespace meta
