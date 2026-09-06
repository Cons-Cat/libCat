// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/math>

namespace cat {

template <is_less_than_comparable T, typename... Remaining>
   requires((is_implicitly_convertible<Remaining, T> && ...))
[[nodiscard]]
constexpr auto
min(T value, Remaining... remaining) -> T {
   ((value = value < remaining ? value : T(remaining)), ...);
   return value;
}

template <is_greater_than_comparable T, typename... Remaining>
   requires((is_implicitly_convertible<Remaining, T> && ...))
[[nodiscard]]
constexpr auto
max(T value, Remaining... remaining) -> T {
   ((value = value > remaining ? value : T(remaining)), ...);
   return value;
}

}  // namespace cat
