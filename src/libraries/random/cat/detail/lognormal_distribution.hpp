// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/math>
#include <cat/random>

#include "./normal_distribution.hpp"

// `cat::lognormal_distribution` models positive values whose logarithms are
// normal with mean `m` and standard deviation `s`, similar to
// `std::lognormal_distribution`. `Float` is the result and parameter type and
// defaults to `float8`. The parameters are read with `.m()` and `.s()`.
//
// This can be used to model positive values driven by multiplicative effects.
// For example, it can model an asset price at a future time.
//
// Example usage:
//
//    pcg_dxsm_engine<uint8> rng;
//    lognormal_distribution<float8> lognormal(0, 1);
//    float8 result = lognormal(rng);
//
// Unlike `std::lognormal_distribution`, this implementation supports SIMD.
// Each normal mean and standard-deviation lane pair produces its own result:
//
//    pcg_dxsm_engine<uint8x2> rng;
//    lognormal_distribution<float8x2> simd_lognormal(
//       float8x2(0), float8x2(1)
//    );
//    float8x2 results = simd_lognormal(rng);
//
// References
//    https://en.cppreference.com/w/cpp/numeric/random/lognormal_distribution

namespace cat {

template <typename Float = float8>
   requires(is_floating_point<Float> || is_simd_floating_point<Float>)
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
