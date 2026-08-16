// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/integer_distribution_helpers.hpp>

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/random>

namespace cat {

// Reference:
// https://en.cppreference.com/w/cpp/numeric/random/geometric_distribution
template <is_integral Int = int4, is_floating_point Float = float8>
   requires(!is_bool<Int>)
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
      return detail::geometric_sample<result_type>(
         parameters.p(), detail::distribution_unit<Float>(generator)
      );
   }

   friend constexpr auto
   operator==(geometric_distribution const&, geometric_distribution const&)
      -> bool = default;

 private:
   param_type m_parameters;
};

}  // namespace cat
