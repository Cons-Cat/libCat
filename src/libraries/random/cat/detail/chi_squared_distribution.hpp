// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/random>

namespace cat {

// https://en.cppreference.com/w/cpp/numeric/random/chi_squared_distribution
template <is_floating_point Float = float8>
class chi_squared_distribution {
 public:
   using result_type = Float;

   class param_type {
    public:
      using distribution_type = chi_squared_distribution;

      constexpr explicit param_type(Float degrees_of_freedom = 1.f)
          : m_degrees_of_freedom(degrees_of_freedom) {
      }

      constexpr auto
      n() const -> Float {
         return m_degrees_of_freedom;
      }

      friend constexpr auto
      operator==(param_type const&, param_type const&) -> bool = default;

    private:
      Float m_degrees_of_freedom;
   };

   constexpr explicit chi_squared_distribution(Float degrees_of_freedom = 1.f)
       : m_parameter(degrees_of_freedom) {
   }

   constexpr explicit chi_squared_distribution(param_type const& parameter)
       : m_parameter(parameter) {
   }

   constexpr void
   reset() {
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
      return detail::random_gamma<Float>(generator, parameter.n() / 2.f, 2.f);
   }

   friend constexpr auto
   operator==(chi_squared_distribution const&, chi_squared_distribution const&)
      -> bool = default;

 private:
   param_type m_parameter;
};

}  // namespace cat
