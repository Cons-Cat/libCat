// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/normal_distribution.hpp>

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/math>
#include <cat/random>

namespace cat {

// https://en.cppreference.com/w/cpp/numeric/random/lognormal_distribution
template <is_floating_point Float = float8>
class lognormal_distribution {
 public:
   using result_type = Float;

   class param_type {
    public:
      using distribution_type = lognormal_distribution;

      constexpr explicit param_type(
         Float mean = 0.f, Float standard_deviation = 1.f
      )
          : m_mean(mean), m_standard_deviation(standard_deviation) {
      }

      constexpr auto
      m() const -> Float {
         return m_mean;
      }

      constexpr auto
      s() const -> Float {
         return m_standard_deviation;
      }

      friend constexpr auto
      operator==(param_type const&, param_type const&) -> bool = default;

    private:
      Float m_mean;
      Float m_standard_deviation;
   };

   constexpr explicit lognormal_distribution(
      Float mean = 0.f, Float standard_deviation = 1.f
   )
       : m_parameter(mean, standard_deviation) {
   }

   constexpr explicit lognormal_distribution(param_type const& parameter)
       : m_parameter(parameter) {
   }

   constexpr void
   reset() {
      m_normal.reset();
   }

   constexpr auto
   m() const -> Float {
      return m_parameter.m();
   }

   constexpr auto
   s() const -> Float {
      return m_parameter.s();
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

   template <is_uniform_random_bit_generator Generator>
   constexpr auto
   operator()(Generator& generator) -> result_type {
      return (*this)(generator, m_parameter);
   }

   template <is_uniform_random_bit_generator Generator>
   constexpr auto
   operator()(Generator& generator, param_type const& parameter)
      -> result_type {
      typename normal_distribution<Float>::param_type normal_parameter(
         parameter.m(), parameter.s()
      );
      return exp(m_normal(generator, normal_parameter));
   }

   friend constexpr auto
   operator==(lognormal_distribution const&, lognormal_distribution const&)
      -> bool = default;

 private:
   param_type m_parameter;
   normal_distribution<Float> m_normal;
};

}  // namespace cat
