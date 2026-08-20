// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/math>
#include <cat/random>

#include "./distribution_helpers.hpp"

// `cat::cauchy_distribution` models heavy-tailed values with location `a` and
// scale `b`, similar to `std::cauchy_distribution`. `Float` is the result and
// parameter type and defaults to `float8`. The parameters are read with `.a()`
// and `.b()`.
//
// This can be used to model heavy-tailed deviations from a central location.
// For example, it can model resonant spectral line shapes.
//
// Example usage:
//
//    pcg_dxsm_engine<uint8> rng;
//    cauchy_distribution<float8> cauchy(0, 1);
//    float8 result = cauchy(rng);
//
// Unlike `std::cauchy_distribution`, this implementation supports SIMD.
// Each location and scale lane pair produces its own value:
//
//    pcg_dxsm_engine<uint8x2> rng;
//    cauchy_distribution<float8x2> simd_cauchy(
//       float8x2(0), float8x2(1)
//    );
//    float8x2 results = simd_cauchy(rng);
//
// References
//    https://en.cppreference.com/w/cpp/numeric/random/cauchy_distribution

namespace cat {

template <typename Float = float8>
   requires(is_floating_point<Float> || is_simd_floating_point<Float>)
class cauchy_distribution {
 public:
   using result_type = Float;

   class param_type {
    public:
      using distribution_type = cauchy_distribution;

      constexpr explicit param_type(Float location = 0.f, Float scale = 1.f)
          : m_location(location), m_scale(scale) {
      }

      constexpr auto
      a() const -> Float {
         return m_location;
      }

      constexpr auto
      b() const -> Float {
         return m_scale;
      }

      friend constexpr auto
      operator==(param_type const&, param_type const&) -> bool = default;

    private:
      Float m_location;
      Float m_scale;
   };

   constexpr explicit cauchy_distribution(
      Float location = 0.f, Float scale = 1.f
   )
       : m_parameter(location, scale) {
   }

   constexpr explicit cauchy_distribution(param_type const& parameter)
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
      return -limits<Float>::infinity();
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
      using scalar = detail::random_scalar<Float>;
      Float const uniform =
         detail::distribution_generate_canonical<Float>(generator);
      return parameter.a()
             + parameter.b() * tan(Float(pi<scalar>) * (uniform - Float(0.5f)));
   }

   friend constexpr auto
   operator==(cauchy_distribution const&, cauchy_distribution const&)
      -> bool = default;

 private:
   param_type m_parameter;
};

}  // namespace cat
