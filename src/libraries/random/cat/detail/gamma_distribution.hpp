// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/random>

// `cat::gamma_distribution` models positive values with shape `alpha` and
// scale `beta`, similar to `std::gamma_distribution`. `Float` is the result
// and parameter type and defaults to `float8`. The parameters are read with
// `.alpha()` and `.beta()`.
//
// This can be used to model total waiting time until multiple independent
// events. For example, it can model time until the third arrival.
//
// Example usage:
//
//    pcg_dxsm_engine<uint8> rng;
//    gamma_distribution<float8> gamma(2, 3);
//    float8 result = gamma(rng);
//
// Unlike `std::gamma_distribution`, this implementation supports SIMD.
// Each shape and scale lane pair produces its own value:
//
//    pcg_dxsm_engine<uint8x2> rng;
//    gamma_distribution<float8x2> simd_gamma(float8x2(2), float8x2(3));
//    float8x2 results = simd_gamma(rng);
//
// References
//    https://en.cppreference.com/w/cpp/numeric/random/gamma_distribution

namespace cat {

template <typename Float = float8>
   requires(is_floating_point<Float> || is_simd_floating_point<Float>)
class gamma_distribution {
 public:
   using result_type = Float;

   class param_type {
    public:
      using distribution_type = gamma_distribution;

      constexpr explicit param_type(Float alpha = 1.f, Float beta = 1.f)
          : m_alpha(alpha), m_beta(beta) {
      }

      constexpr auto
      alpha() const -> Float {
         return m_alpha;
      }

      constexpr auto
      beta() const -> Float {
         return m_beta;
      }

      friend constexpr auto
      operator==(param_type const&, param_type const&) -> bool = default;

    private:
      Float m_alpha;
      Float m_beta;
   };

   constexpr explicit gamma_distribution(Float alpha = 1.f, Float beta = 1.f)
       : m_parameter(alpha, beta) {
   }

   constexpr explicit gamma_distribution(param_type const& parameter)
       : m_parameter(parameter) {
   }

   constexpr void
   reset() {
   }

   constexpr auto
   alpha() const -> Float {
      return m_parameter.alpha();
   }

   constexpr auto
   beta() const -> Float {
      return m_parameter.beta();
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
      return detail::random_gamma(
         generator, parameter.alpha(), parameter.beta()
      );
   }

   friend constexpr auto
   operator==(gamma_distribution const&, gamma_distribution const&)
      -> bool = default;

 private:
   param_type m_parameter;
};

}  // namespace cat
