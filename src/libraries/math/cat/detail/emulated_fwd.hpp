// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>

namespace cat::detail {

template <is_raw_floating_point Float>
[[nodiscard]]
constexpr auto
emulated_exp(Float argument) noexcept -> Float;

template <is_raw_floating_point Float>
[[nodiscard]]
constexpr auto
emulated_log(Float argument) noexcept -> Float;

template <is_raw_floating_point Float>
[[nodiscard]]
constexpr auto
emulated_sin(Float argument) noexcept -> Float;

}  // namespace cat::detail
