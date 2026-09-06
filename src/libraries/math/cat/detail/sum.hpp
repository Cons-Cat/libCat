// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/math>

namespace cat {

template <is_arithmetic T, is_arithmetic... Remaining>
[[nodiscard]]
constexpr auto
sum(T value, Remaining... remaining) {
   return (value + ... + remaining);
}

}  // namespace cat
