// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/uniform_float_distribution.hpp>

#include <cat/arithmetic>
#include <cat/array>
#include <cat/math>
#include <cat/random>
#include <cat/span>

namespace cat {

namespace detail {

template <is_floating_point Float, uniform_random_bit_generator Generator>
constexpr auto
sampling_unit(Generator& generator) -> Float {
   uniform_float_distribution<Float> distribution;
   return distribution(generator);
}

template <is_floating_point Float>
constexpr auto
sampling_finite(Float value) -> bool {
   return is_finite(value);
}

template <is_floating_point Float>
constexpr auto
sampling_interval(
   array<Float, sampling_distribution_capacity> const& cumulative, idx size,
   Float value
) -> idx {
   idx first = 0u;
   idx count = size;
   while (count != 0u) {
      idx const step = count / 2u;
      idx const middle = first + step;
      if (value < cumulative[middle]) {
         count = step;
      } else {
         first = middle + 1u;
         count = idx(count - step - 1u);
      }
   }
   return first == size ? idx(size - 1u) : first;
}

template <is_integral Int>
constexpr auto
sampling_integer(idx value) -> Int {
   return Int(static_cast<raw_arithmetic_type<Int>>(value));
}

}  // namespace detail

}  // namespace cat
