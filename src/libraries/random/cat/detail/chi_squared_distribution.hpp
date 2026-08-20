// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/random>

// `cat::chi_squared_distribution` models a sum of squared standard normal
// values with `n` degrees of freedom, similar to
// `std::chi_squared_distribution`. `Float` is the result and parameter type
// and defaults to `float8`. The degrees of freedom are read with `.n()`.
//
// This can be used to model variability estimated from normally distributed
// samples. For example, it can model a scaled sample-variance statistic.
//
// Example usage:
//
//    pcg_dxsm_engine<uint8> rng;
//    chi_squared_distribution<float8> chi_squared(4);
//    float8 result = chi_squared(rng);
//
// Unlike `std::chi_squared_distribution`, this implementation supports SIMD.
// Each degrees-of-freedom lane produces its own statistic:
//
//    pcg_dxsm_engine<uint8x2> rng;
//    chi_squared_distribution<float8x2> simd_chi_squared(float8x2(4));
//    float8x2 results = simd_chi_squared(rng);
//
// References
//    https://en.cppreference.com/w/cpp/numeric/random/chi_squared_distribution

namespace cat {

template <typename Float = float8>
   requires(is_floating_point<Float> || is_simd_floating_point<Float>)
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
      return limits<Float>::infinity();
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
