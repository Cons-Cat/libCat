// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/random>

namespace cat {

// https://en.cppreference.com/w/cpp/numeric/random/fisher_f_distribution
template <is_floating_point Float = float8>
class fisher_f_distribution {
 public:
   using result_type = Float;

   class param_type {
    public:
      using distribution_type = fisher_f_distribution;

      constexpr explicit param_type(
         Float numerator_degrees = 1.f, Float denominator_degrees = 1.f
      )
          : m_numerator_degrees(numerator_degrees),
            m_denominator_degrees(denominator_degrees) {
      }

      constexpr auto
      m() const -> Float {
         return m_numerator_degrees;
      }

      constexpr auto
      n() const -> Float {
         return m_denominator_degrees;
      }

      friend constexpr auto
      operator==(param_type const&, param_type const&) -> bool = default;

    private:
      Float m_numerator_degrees;
      Float m_denominator_degrees;
   };

   constexpr explicit fisher_f_distribution(
      Float numerator_degrees = 1.f, Float denominator_degrees = 1.f
   )
       : m_parameter(numerator_degrees, denominator_degrees) {
   }

   constexpr explicit fisher_f_distribution(param_type const& parameter)
       : m_parameter(parameter) {
   }

   constexpr void
   reset() {
   }

   constexpr auto
   m() const -> Float {
      return m_parameter.m();
   }

   constexpr auto
   n() const -> Float {
      return m_parameter.n();
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
      Float const numerator =
         detail::random_gamma<Float>(generator, parameter.m() / 2.f, 2.f)
         / parameter.m();
      Float const denominator =
         detail::random_gamma<Float>(generator, parameter.n() / 2.f, 2.f)
         / parameter.n();
      return numerator / denominator;
   }

   friend constexpr auto
   operator==(fisher_f_distribution const&, fisher_f_distribution const&)
      -> bool = default;

 private:
   param_type m_parameter;
};

}  // namespace cat
