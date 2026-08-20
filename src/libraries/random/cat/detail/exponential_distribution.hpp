// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/math>
#include <cat/random>

// `cat::exponential_distribution` models waiting time between independent
// events occurring at rate `lambda`, similar to
// `std::exponential_distribution`. `Float` is the result and parameter type
// and defaults to `float8`. The rate is read with `.lambda()`.
//
// This can be used to model waiting time between independent events. For
// example, it can model time until the next service request.
//
// Example usage:
//
//    pcg_dxsm_engine<uint8> rng;
//    exponential_distribution<float8> exponential(2);
//    float8 result = exponential(rng);
//
// Unlike `std::exponential_distribution`, this implementation supports SIMD.
// Each rate lane produces its own waiting time:
//
//    pcg_dxsm_engine<uint8x2> rng;
//    exponential_distribution<float8x2> simd_exponential(float8x2(2));
//    float8x2 results = simd_exponential(rng);
//
// References
//    https://en.cppreference.com/w/cpp/numeric/random/exponential_distribution

namespace cat {

template <typename Float = float8>
   requires(is_floating_point<Float> || is_simd_floating_point<Float>)
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
