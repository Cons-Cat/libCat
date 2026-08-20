// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/math>
#include <cat/random>

#include "./uniform_float_distribution.hpp"

namespace cat::detail {

template <typename Float, is_uniform_random_bit_generator Generator>
   requires(is_floating_point<Float> || is_simd_floating_point<Float>)
constexpr auto
distribution_unit(Generator& generator) -> Float {
   return uniform_float_distribution<Float>{}(generator);
}

template <typename Int, typename Float>
   requires(
      (is_integral<Int> || is_simd_integral<Int>)
      && (is_floating_point<Float> || is_simd_floating_point<Float>)
   )
constexpr auto
distribution_integer(Float value) -> Int {
   Float const maximum = distribution_float<Float>(
      make_raw_arithmetic(limits<random_scalar<Int>>::max())
   );
   if constexpr (is_simd<Int>) {
      Int const converted(
         __builtin_convertvector(value.raw, typename Int::raw_type)
      );
      return simd_select(
         distribution_mask_cast<Int>(value >= maximum), limits<Int>::max(),
         simd_select(
            distribution_mask_cast<Int>(value <= Float(0)), Int(0), converted
         )
      );
   } else {
      if (value >= maximum) {
         return limits<Int>::max();
      }
      if (value <= distribution_float<Float>(0)) {
         return 0u;
      }
      return Int(value);
   }
}

template <typename Int, typename Float>
   requires(
      (is_integral<Int> || is_simd_integral<Int>)
      && (is_floating_point<Float> || is_simd_floating_point<Float>)
   )
constexpr auto
geometric_sample(Float probability, Float unit) -> Int {
   Float const one = distribution_float<Float>(1);
   Float const zero = distribution_float<Float>(0);
   Float const value = floor(log(one - unit) / log(one - probability));
   if constexpr (is_simd<Int>) {
      return simd_select(
         distribution_mask_cast<Int>(probability >= one || unit <= zero),
         Int(0), distribution_integer<Int>(value)
      );
   } else {
      if (probability >= one || unit <= zero) {
         return 0u;
      }
      return distribution_integer<Int>(value);
   }
}

template <typename Int>
   requires(is_integral<Int> || is_simd_integral<Int>)
constexpr void
distribution_add(Int& result, Int value) {
   if constexpr (is_simd<Int>) {
      Int const maximum = limits<Int>::max();
      result = simd_select(value > maximum - result, maximum, result + value);
   } else {
      if (value > limits<Int>::max() - result) {
         result = limits<Int>::max();
      } else {
         result += value;
      }
   }
}

}  // namespace cat::detail
