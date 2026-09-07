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

}  // namespace std

// NOLINTEND(bugprone-std-namespace-modification)
