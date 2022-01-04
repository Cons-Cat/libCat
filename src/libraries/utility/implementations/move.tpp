// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <utility>

template <typename T>
constexpr auto meta::move(T&& input) -> meta::remove_reference_t<T>&& {
    return static_cast<meta::remove_reference_t<T>&&>(input);
}
