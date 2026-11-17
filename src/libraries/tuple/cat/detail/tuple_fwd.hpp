// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// NOLINTBEGIN(bugprone-std-namespace-modification)
namespace std {

template <typename>
struct tuple_size;

template <typename T>
inline constexpr __SIZE_TYPE__ tuple_size_v = tuple_size<T>::value;

template <__SIZE_TYPE__, typename>
struct tuple_element;

template <__SIZE_TYPE__ index, typename T>
using tuple_element_t = tuple_element<index, T>::type;

// Like the standard library, every tuple-like type is also tuple-like when
// cv-qualified, so that it can be bound with `auto const [...]`. These are only
// defined when `T` is tuple-like.
// https://eel.is/c++draft/tuple.helper
// The `volatile` forms are deprecated by P1831 and proposed for removal by
// P2866, but standard libraries still provide them.

template <typename T>
   requires(requires { tuple_size<T>::value; })
struct tuple_size<T const> : tuple_size<T> {};

template <typename T>
   requires(requires { tuple_size<T>::value; })
struct tuple_size<T volatile> : tuple_size<T> {};

template <typename T>
   requires(requires { tuple_size<T>::value; })
struct tuple_size<T const volatile> : tuple_size<T> {};

template <__SIZE_TYPE__ index, typename T>
   requires(requires { typename tuple_element<index, T>::type; })
struct tuple_element<index, T const> {
   using type = tuple_element<index, T>::type const;
};

template <__SIZE_TYPE__ index, typename T>
   requires(requires { typename tuple_element<index, T>::type; })
struct tuple_element<index, T volatile> {
   using type = tuple_element<index, T>::type volatile;
};

template <__SIZE_TYPE__ index, typename T>
   requires(requires { typename tuple_element<index, T>::type; })
struct tuple_element<index, T const volatile> {
   using type = tuple_element<index, T>::type const volatile;
};

}  // namespace std

// NOLINTEND(bugprone-std-namespace-modification)
