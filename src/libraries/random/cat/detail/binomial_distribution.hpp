// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/random>

#include "./distribution_helpers.hpp"
#include "./integer_distribution_helpers.hpp"

// `cat::binomial_distribution` models the number of successes in `t`
// independent trials with success probability `p`, similar to
// `std::binomial_distribution`. `Int` is the result type and defaults to
// `int4`. `Float` stores `p` and defaults to `float8`. The parameters are read
// with `.t()` and `.p()`.
//
// This can be used to count successes across a fixed number of independent
// trials. For example, it can model defective items in a production batch.
//
// Example usage:
//
//    pcg_dxsm_engine<uint8> rng;
//    binomial_distribution<int4, float8> binomial(10, 0.25);
//    int4 result = binomial(rng);
//
// Unlike `std::binomial_distribution`, this implementation supports SIMD.
// Each trial-count lane produces its own result using the shared `p`:
//
//    pcg_dxsm_engine<uint4x4> rng;
//    binomial_distribution<int4x4> simd_binomial(int4x4(10), 0.25);
//    int4x4 results = simd_binomial(rng);
//
// References
//    https://en.cppreference.com/w/cpp/numeric/random/binomial_distribution

namespace cat {

template <typename Int = int4, is_floating_point Float = float8>
   requires((is_integral<Int> || is_simd_integral<Int>) && !is_bool<Int>)
class binomial_distribution {
 public:
   using result_type = Int;

   class param_type {
    public:
      using distribution_type = binomial_distribution;

      constexpr explicit param_type(
         result_type trials = 1, Float probability = 0.5
      )
          : m_trials(trials), m_probability(probability) {
      }

      constexpr auto
      t() const -> result_type {
         return m_trials;
      }

      constexpr auto
      p() const -> Float {
         return m_probability;
      }

      friend constexpr auto
      operator==(param_type const&, param_type const&) -> bool = default;

    private:
      result_type m_trials;
      Float m_probability;
   };

   constexpr explicit binomial_distribution(
      result_type trials = 1, Float probability = 0.5
   )
       : m_parameters(trials, probability) {
   }

   constexpr explicit binomial_distribution(param_type parameters)
       : m_parameters(parameters) {
   }

   constexpr void
   reset() {
   }

   constexpr auto
   t() const -> result_type {
      return m_parameters.t();
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
      return t();
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
      result_type remaining = parameters.t();
      using sample_type = detail::distribution_float_result<result_type, Float>;
      sample_type const probability =
         detail::distribution_broadcast<sample_type>(parameters.p());
      if constexpr (is_simd<result_type>) {
         auto active = remaining > result_type(0);
         while (detail::distribution_any(active)) {
            auto const sampled =
               detail::distribution_unit<sample_type>(generator) < probability;
            auto const success =
               active && detail::distribution_mask_cast<result_type>(sampled);
            result += simd_select(success, result_type(1), result_type(0));
            remaining -= simd_select(active, result_type(1), result_type(0));
            active = remaining > result_type(0);
         }
      } else {
         while (remaining > 0) {
            if (detail::distribution_unit<Float>(generator) < probability) {
               ++result;
            }
            --remaining;
         }
      }
      return result;
   }

   friend constexpr auto
   operator==(binomial_distribution const&, binomial_distribution const&)
      -> bool = default;

 private:
   param_type m_parameters;
};

}  // namespace cat
