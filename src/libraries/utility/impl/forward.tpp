// -*- mode: c++ -*-
// vim: set ft=cpp:

template <typename T>
constexpr auto meta::forward(meta::remove_reference_t<T>& input) -> T&& {
    return static_cast<T&&>(input);
}

template <typename T>
constexpr auto meta::forward(meta::remove_reference_t<T>&& input)
    -> T&& requires(!meta::is_lvalue_reference_v<T>) {
    return static_cast<T&&>(input);
}
