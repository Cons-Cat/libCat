// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/math>
#include <cat/random>

// `cat::weibull_distribution` models nonnegative lifetimes with shape `a` and
// scale `b`, similar to `std::weibull_distribution`. `Float` is the result and
// parameter type and defaults to `float8`. The parameters are read with `.a()`
// and `.b()`.
//
// This can be used to model lifetimes with increasing or decreasing failure
// rates. For example, it can model component failure times.
//
// Example usage:
//
//    pcg_dxsm_engine<uint8> rng;
//    weibull_distribution<float8> weibull(2, 3);
//    float8 result = weibull(rng);
//
// Unlike `std::weibull_distribution`, this implementation supports SIMD.
// Each shape and scale lane pair produces its own lifetime:
//
//    pcg_dxsm_engine<uint8x2> rng;
//    weibull_distribution<float8x2> simd_weibull(float8x2(2), float8x2(3));
//    float8x2 results = simd_weibull(rng);
//
// References
//    https://en.cppreference.com/w/cpp/numeric/random/weibull_distribution

namespace cat {

template <typename Float = float8>
   requires(is_floating_point<Float> || is_simd_floating_point<Float>)
class weibull_distribution {
 public:
   using result_type = Float;

   class param_type {
    public:
      using distribution_type = weibull_distribution;

      constexpr explicit param_type(Float shape = 1.f, Float scale = 1.f)
          : m_shape(shape), m_scale(scale) {
      }

      constexpr auto
      a() const -> Float {
         return m_shape;
      }

      constexpr auto
      b() const -> Float {
         return m_scale;
      }

      friend constexpr auto
      operator==(param_type const&, param_type const&) -> bool = default;

    private:
      Float m_shape;
      Float m_scale;
   };

   constexpr explicit weibull_distribution(Float shape = 1.f, Float scale = 1.f)
       : m_parameter(shape, scale) {
   }

   constexpr explicit weibull_distribution(param_type const& parameter)
       : m_parameter(parameter) {
   }

   constexpr void
   reset() {
   }

   constexpr auto
   a() const -> Float {
      return m_parameter.a();
   }

   constexpr auto
   b() const -> Float {
      return m_parameter.b();
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
      Float const exponential =
         -log(detail::random_positive_canonical<Float>(generator));
      Float const one = 1.f;
      return parameter.b()
             * detail::distribution_pow(exponential, one / parameter.a());
   }

   friend constexpr auto
   operator==(weibull_distribution const&, weibull_distribution const&)
      -> bool = default;

 private:
   param_type m_parameter;
};

}  // namespace cat
