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

template <is_floating_point Float>
constexpr auto
distribution_float(auto value) -> Float {
   return Float(raw_arithmetic_type<Float>(value));
}

template <is_floating_point Float, is_uniform_random_bit_generator Generator>
constexpr auto
distribution_unit(Generator& generator) -> Float {
   return uniform_float_distribution<Float>{}(generator);
}

template <is_integral Int, is_floating_point Float>
constexpr auto
distribution_integer(Float value) -> Int {
   Float const maximum =
      distribution_float<Float>(make_raw_arithmetic(limits<Int>::max()));
   if (value >= maximum) {
      return limits<Int>::max();
   }
   if (value <= distribution_float<Float>(0)) {
      return 0u;
   }
   return Int(value);
}

template <is_integral Int, is_floating_point Float>
constexpr auto
geometric_sample(Float probability, Float unit) -> Int {
   Float const one = distribution_float<Float>(1);
   Float const zero = distribution_float<Float>(0);
   if (probability >= one || unit <= zero) {
      return 0u;
   }
   Float const value = floor(log(one - unit) / log(one - probability));
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
