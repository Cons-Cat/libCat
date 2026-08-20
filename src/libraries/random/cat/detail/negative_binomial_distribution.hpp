// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/random>

#include "./distribution_helpers.hpp"
#include "./integer_distribution_helpers.hpp"

// `cat::negative_binomial_distribution` models the number of failures before
// `k` successes when each trial succeeds with probability `p`, similar to
// `std::negative_binomial_distribution`. `Int` is the result type and defaults
// to `int4`. `Float` stores `p` and defaults to `float8`. The parameters are
// read with `.k()` and `.p()`.
//
// This can be used to count failures before a fixed number of successes. For
// example, it can model unsuccessful calls before reaching a sales quota.
//
// Example usage:
//
//    pcg_dxsm_engine<uint8> rng;
//    negative_binomial_distribution<int4, float8> negative_binomial(3, 0.5);
//    int4 result = negative_binomial(rng);
//
// Unlike `std::negative_binomial_distribution`, this implementation supports
// SIMD. Each success-count lane produces its own result using the shared `p`:
//
//    pcg_dxsm_engine<uint4x4> rng;
//    negative_binomial_distribution<int4x4> simd_negative_binomial(
//       int4x4(3), 0.5
//    );
//    int4x4 results = simd_negative_binomial(rng);
//
// References
//    https://en.cppreference.com/w/cpp/numeric/random/negative_binomial_distribution

namespace cat {

template <typename Int = int4, is_floating_point Float = float8>
   requires((is_integral<Int> || is_simd_integral<Int>) && !is_bool<Int>)
class negative_binomial_distribution {
 public:
   using result_type = Int;

   class param_type {
    public:
      using distribution_type = negative_binomial_distribution;

      constexpr explicit param_type(
         result_type successes = 1, Float probability = 0.5
      )
          : m_successes(successes), m_probability(probability) {
      }

      constexpr auto
      k() const -> result_type {
         return m_successes;
      }

      constexpr auto
      p() const -> Float {
         return m_probability;
      }

      friend constexpr auto
      operator==(param_type const&, param_type const&) -> bool = default;

    private:
      result_type m_successes;
      Float m_probability;
   };

   constexpr explicit negative_binomial_distribution(
      result_type successes = 1, Float probability = 0.5
   )
       : m_parameters(successes, probability) {
   }

   constexpr explicit negative_binomial_distribution(param_type parameters)
       : m_parameters(parameters) {
   }

   constexpr void
   reset() {
   }

   constexpr auto
   k() const -> result_type {
      return m_parameters.k();
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
      result_type result = 0u;
      result_type remaining = parameters.k();
      using sample_type = detail::distribution_float_result<result_type, Float>;
      sample_type const probability =
         detail::distribution_broadcast<sample_type>(parameters.p());
      if constexpr (is_simd<result_type>) {
         auto active = remaining > result_type(0);
         while (detail::distribution_any(active)) {
            result_type const sample = detail::geometric_sample<result_type>(
               probability, detail::distribution_unit<sample_type>(generator)
            );
            detail::distribution_add(
               result, simd_select(active, sample, result_type(0))
            );
            remaining -= simd_select(active, result_type(1), result_type(0));
            active = remaining > result_type(0);
         }
      } else {
         while (remaining > 0) {
            detail::distribution_add(
               result,
               detail::geometric_sample<result_type>(
                  probability, detail::distribution_unit<Float>(generator)
               )
            );
            --remaining;
         }
      }
      return result;
   }

   friend constexpr auto
   operator==(
      negative_binomial_distribution const&,
      negative_binomial_distribution const&
   ) -> bool = default;

 private:
   param_type m_parameters;
};

}  // namespace cat
