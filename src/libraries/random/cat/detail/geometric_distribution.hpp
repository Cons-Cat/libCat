// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/random>

#include "./distribution_helpers.hpp"
#include "./integer_distribution_helpers.hpp"

// `cat::geometric_distribution` models the number of failures before the
// first success when each trial succeeds with probability `p`, similar to
// `std::geometric_distribution`. `Int` is the result type and defaults to
// `int4`. `Float` stores `p` and defaults to `float8`. The probability is read
// with `.p()`.
//
// This can be used to count failures before the first success. For example, it
// can model failed attempts before the first successful connection.
//
// Example usage:
//
//    pcg_dxsm_engine<uint8> rng;
//    geometric_distribution<int4, float8> geometric(0.25);
//    int4 result = geometric(rng);
//
// Unlike `std::geometric_distribution`, this implementation supports SIMD.
// Each engine lane produces its own result using the shared `p`:
//
//    pcg_dxsm_engine<uint4x4> rng;
//    geometric_distribution<int4x4> simd_geometric(0.25);
//    int4x4 results = simd_geometric(rng);
//
// References
//    https://en.cppreference.com/w/cpp/numeric/random/geometric_distribution

namespace cat {

template <typename Int = int4, is_floating_point Float = float8>
   requires((is_integral<Int> || is_simd_integral<Int>) && !is_bool<Int>)
class geometric_distribution {
 public:
   using result_type = Int;

   class param_type {
    public:
      using distribution_type = geometric_distribution;

      constexpr explicit param_type(Float probability = 0.5)
          : m_probability(probability) {
      }

      constexpr auto
      p() const -> Float {
         return m_probability;
      }

      friend constexpr auto
      operator==(param_type const&, param_type const&) -> bool = default;

    private:
      Float m_probability;
   };

   constexpr explicit geometric_distribution(Float probability = 0.5)
       : m_parameters(probability) {
   }

   constexpr explicit geometric_distribution(param_type parameters)
       : m_parameters(parameters) {
   }

   constexpr void
   reset() {
   }

   constexpr auto
   p() const -> Float {
      return m_parameters.p();
   }

   constexpr auto
   param() const -> param_type {
      return m_parameters;
   }

   constexpr void
   param(param_type parameters) {
      m_parameters = parameters;
   }

   constexpr auto
   min() const -> result_type {
      return 0u;
   }

   constexpr auto
   max() const -> result_type {
      return limits<result_type>::max();
   }

   template <is_uniform_random_bit_generator Generator>
   constexpr auto
   operator()(Generator& generator) -> result_type {
      return (*this)(generator, m_parameters);
   }

   template <is_uniform_random_bit_generator Generator>
   constexpr auto
   operator()(Generator& generator, param_type const& parameters)
      -> result_type {
      using sample_type = detail::distribution_float_result<result_type, Float>;
      return detail::geometric_sample<result_type>(
         detail::distribution_broadcast<sample_type>(parameters.p()),
         detail::distribution_unit<sample_type>(generator)
      );
   }

   friend constexpr auto
   operator==(geometric_distribution const&, geometric_distribution const&)
      -> bool = default;

 private:
   param_type m_parameters;
};

}  // namespace cat
