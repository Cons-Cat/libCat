// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/integer_distribution_helpers.hpp>

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/random>

namespace cat {

// Reference:
// https://en.cppreference.com/w/cpp/numeric/random/negative_binomial_distribution
template <is_integral Int = int4, is_floating_point Float = float8>
   requires(!is_bool<Int>)
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

   template <uniform_random_bit_generator Generator>
   constexpr auto
   operator()(Generator& generator) -> result_type {
      return (*this)(generator, m_parameters);
   }

   template <uniform_random_bit_generator Generator>
   constexpr auto
   operator()(Generator& generator, param_type const& parameters)
      -> result_type {
      result_type result = 0u;
      result_type remaining = parameters.k();
      Float const probability = parameters.p();
      while (remaining > 0) {
         detail::distribution_add(
            result, detail::geometric_sample<result_type>(
                       probability, detail::distribution_unit<Float>(generator)
                    )
         );
         --remaining;
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
