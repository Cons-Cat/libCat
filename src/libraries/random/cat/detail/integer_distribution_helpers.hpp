// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/uniform_float_distribution.hpp>

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/math>
#include <cat/random>

namespace cat {

namespace detail {

template <is_floating_point Float, uniform_random_bit_generator Generator>
constexpr auto
distribution_unit(Generator& generator) -> Float {
   return uniform_float_distribution<Float>{}(generator);
}

template <is_integral Int, is_floating_point Float>
constexpr auto
distribution_integer(Float value) -> Int {
   Float const maximum = Float(limits<Int>::max());
   if (value >= maximum) {
      return limits<Int>::max();
   }
   if (value <= 0.0) {
      return 0u;
   }
   return static_cast<Int>(value);
}

template <is_integral Int, is_floating_point Float>
constexpr auto
geometric_sample(Float probability, Float unit) -> Int {
   if (probability >= 1.0 || unit <= 0.0) {
      return 0u;
   }
   Float const value = floor(log(1.f - unit) / log(1.f - probability));
   return distribution_integer<Int>(value);
}

template <is_integral Int>
constexpr void
distribution_add(Int& result, Int value) {
   if (value > limits<Int>::max() - result) {
      result = limits<Int>::max();
   } else {
      result += value;
   }
}

}  // namespace detail

}  // namespace cat
