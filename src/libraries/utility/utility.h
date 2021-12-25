// -*- mode: c++ -*-
// vim: set ft=cpp:
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

constexpr auto is_constant_evaluated() -> bool {
    return __builtin_is_constant_evaluated();
}

// TODO: add a meta::invocable concept.
consteval auto constant_evaluate(auto value) {
    return value();
}

/* As far as I can prove, this is a zero-overhead abstraction on -O3.
 * Copy data at some memory byte-by-byte into a differently-typed variable at
 * its own address. Compilers are good at optimizing out this pattern. */
template <typename T>
__attribute__((optimize("-O3"))) auto bit_cast(
    auto& from_value)  // NOLINT
                       // If is a non-const void*
    requires(meta::is_same_v<void*&, decltype(from_value)>) {
    char* p_from = static_cast<char*>(from_value);
    T* p_to = static_cast<T*>(static_cast<void*>(p_from));
    for (unsigned i = 0; i < sizeof(T); i++) {
        static_cast<char*>(static_cast<void*>(p_to))[i] = p_from[i];
    }
    return *p_to;
}

template <typename T>
__attribute__((optimize("-O3"))) auto bit_cast(
    auto& from_value)  // NOLINT
                       // If is a void* const
    requires(meta::is_same_v<void* const&, decltype(from_value)>) {
    char* p_from = static_cast<char*>(const_cast<void*>(from_value));
    T* p_to = static_cast<T*>(static_cast<void*>(p_from));
    for (unsigned i = 0; i < sizeof(T); i++) {
        static_cast<char*>(static_cast<void*>(p_to))[i] = p_from[i];
    }
    return *p_to;
}

template <typename T>
__attribute__((optimize("-O3"))) auto bit_cast(
    auto& from_value)  // NOLINT
                       // If not a void*, and not const:
    requires(
        !meta::is_same_v<void*&, decltype(from_value)> &&
        !meta::is_const_v<meta::remove_reference_t<decltype(from_value)>>) {
    char* p_from = static_cast<char*>(static_cast<void*>(&from_value));
    T* p_to = static_cast<T*>(static_cast<void*>(p_from));
    for (unsigned i = 0; i < sizeof(T); i++) {
        static_cast<char*>(static_cast<void*>(p_to))[i] = p_from[i];
    }
    return *p_to;
}

template <typename T>
__attribute__((optimize("-O3"))) auto bit_cast(
    auto& from_value)  // NOLINT
                       // If not a void* const, and is const:
    requires(!meta::is_same_v<void* const&, decltype(from_value)> &&
             meta::is_const_v<meta::remove_reference_t<decltype(from_value)>>) {
    char* p_from = static_cast<char*>(const_cast<void*>(&from_value));
    T* p_to = static_cast<T*>(static_cast<void*>(p_from));
    for (unsigned i = 0; i < sizeof(T); i++) {
        static_cast<char*>(static_cast<void*>(p_to))[i] = p_from[i];
    }
    return *p_to;
}

}  // namespace meta
