// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/math>
#include <cat/random>

namespace cat {

// https://en.cppreference.com/w/cpp/numeric/random/exponential_distribution
template <is_floating_point Float = float8>
class exponential_distribution {
 public:
   using result_type = Float;

   class param_type {
    public:
      using distribution_type = exponential_distribution;

      constexpr explicit param_type(Float lambda = 1.f) : m_lambda(lambda) {
      }

      constexpr auto
      lambda() const -> Float {
         return m_lambda;
      }

      friend constexpr auto
      operator==(param_type const&, param_type const&) -> bool = default;

    private:
      Float m_lambda;
   };

   constexpr explicit exponential_distribution(Float lambda = 1.f)
       : m_parameter(lambda) {
   }

   constexpr explicit exponential_distribution(param_type const& parameter)
       : m_parameter(parameter) {
   }

   constexpr void
   reset() {
   }

   constexpr auto
   lambda() const -> Float {
      return m_parameter.lambda();
   }

   constexpr auto
   param() const -> param_type {
      return m_parameter;
   }

   constexpr void
   param(param_type const& parameter) {
      m_parameter = parameter;
   }

   static constexpr auto
   min() -> result_type {
      return 0.f;
   }

   static constexpr auto
   max() -> result_type {
      return limits<Float>::max();
   }

   template <uniform_random_bit_generator Generator>
   constexpr auto
   operator()(Generator& generator) -> result_type {
      return (*this)(generator, m_parameter);
   }

   template <uniform_random_bit_generator Generator>
   constexpr auto
   operator()(Generator& generator, param_type const& parameter)
      -> result_type {
      return -log(detail::random_positive_canonical<Float>(generator))
             / parameter.lambda();
   }

   friend constexpr auto
   operator==(exponential_distribution const&, exponential_distribution const&)
      -> bool = default;

 private:
   param_type m_parameter;
};

}  // namespace cat
