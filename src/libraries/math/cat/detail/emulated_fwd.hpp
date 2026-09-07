// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>

namespace cat::detail {

template <is_raw_floating_point Float>
[[nodiscard]]
constexpr auto
emulated_exp(Float argument) -> Float;

template <is_raw_floating_point Float>
[[nodiscard]]
constexpr auto
emulated_log(Float argument) -> Float;

template <is_raw_floating_point Float>
[[nodiscard]]
constexpr auto
emulated_sin(Float argument) -> Float;

}  // namespace cat::detail
